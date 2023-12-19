#include "vjoule_api.hh"
#include "intern/files.hh"
#include "intern/signal.hh"

#include <sstream>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <sys/inotify.h>

using namespace common;


#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define EVENT_BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

concurrency::signal<> __exit_signal__;

namespace vjoule {
      
    vjoule_api::vjoule_api () {	
        __exit_signal__.connect (this, &vjoule_api::on_exit);
	
        auto sys = system ("systemctl is-active --quiet vjoule_service.service");
        if (sys != 0) {
            throw vjoule::vjoule_error ("vJoule service is not started. sudo systemctl start vjoule_service.service");
        }

        this-> _inotifFd = inotify_init ();
        this-> _inotifFdW = inotify_add_watch (this-> _inotifFd, utils::join_path (VJOULE_DIR, "results").c_str (), IN_MODIFY | IN_CREATE);

        this-> force_sig ();
	
    }

    consumption_stamp_t vjoule_api::get_machine_current_consumption () const {
        this-> force_sig ();
        consumption_stamp_t t (std::chrono::system_clock::now (), 0, 0, 0);
        auto cpu = utils::join_path (VJOULE_DIR, "results/cpu");
        auto gpu = utils::join_path (VJOULE_DIR, "results/gpu");
        auto ram = utils::join_path (VJOULE_DIR, "results/ram");
        auto pdu = utils::join_path (VJOULE_DIR, "results/pdu");
        {
            std::ifstream f (cpu);
            if (f.is_open ()) {
                f >> t.cpu;
            }
        }
	
        {
            std::ifstream f (gpu);
            if (f.is_open ()) {
                f >> t.gpu;
            }
        }
	
        {
            std::ifstream f (ram);
            if (f.is_open ()) {
                f >> t.ram;
            }
        }

        {
            std::ifstream f (pdu);
            if (f.is_open ()) {
                f >> t.pdu;
            }
        }

        return t;
    }

    void vjoule_api::force_sig () const {
        for (uint64_t i = 0 ; i < 2 ; i++) {
            FILE * f = fopen (utils::join_path (VJOULE_DIR, "signal").c_str (), "w");
            if (f == nullptr) throw vjoule::vjoule_error ("failed to signal service.");
            fprintf (f, "%d\n", 1);
            fflush (f);
            fclose (f);
	    
            usleep (10000);
            this-> wait_iteration ();
        }
    }

    void vjoule_api::wait_iteration () const {
        char buffer[EVENT_BUF_LEN];
        while (true) {
            auto len = read (this-> _inotifFd, buffer, EVENT_BUF_LEN);
            if (len == 0) {
                throw std::runtime_error ("reading cgroup notif");
            }

            int i = 0;
            while (i < len) {
                struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];
                if (event-> len != 0) { // there is an event to read
                    if (event-> mask & IN_CREATE || event-> mask & IN_DELETE | event-> mask & IN_MODIFY) {
                        return;
                    }
                }
                i += EVENT_SIZE + event->len;
            }
        }
    }

    void vjoule_api::on_exit () {
        if (this-> _inotifFdW != 0) {
            inotify_rm_watch (this-> _inotifFd, this-> _inotifFdW);
            close (this-> _inotifFd);
            this-> _inotifFd = 0;
            this-> _inotifFdW = 0;
        }
    }
    
    
    vjoule_api::~vjoule_api () {
        this-> on_exit ();
    }    

}


std::ostream & operator << (std::ostream & o, const vjoule::consumption_stamp_t & c) {
    char buffer[255];
    snprintf (buffer, 255, "stamp (%ld, pdu: %.2fJ, cpu: %.2fJ, ram: %.2fJ, gpu: %.2fJ)",
              c.timestamp.time_since_epoch ().count (),
              c.pdu,
              c.cpu,
              c.ram,
              c.gpu);
    
    o << buffer;
    return o;
}

std::ostream & operator << (std::ostream & o, const vjoule::consumption_diff_t & c) {
    char buffer[255];
    snprintf (buffer, 255, "diff (time: %.2fs, pdu: %.2fJ, cpu: %.2fJ, ram: %.2fJ, gpu: %.2fJ)",
              c.duration,
              c.pdu,
              c.cpu,
              c.ram,
              c.gpu);
    
    o << buffer;

    return o;
}

std::ostream & operator << (std::ostream & o, const vjoule::consumption_perc_t & c) {
    char buffer[255];
    snprintf (buffer, 255, "perc (time: %.2f%c, pdu: %.2f%c, cpu: %.2f%c, ram: %.2f%c, gpu: %.2f%c)",
              c.duration, '%',
              c.pdu, '%',
              c.cpu, '%',
              c.ram, '%',
              c.gpu, '%');
    
    o << buffer;

    return o;
}

vjoule::consumption_diff_t operator- (const vjoule::consumption_stamp_t & left, const vjoule::consumption_stamp_t & right) {
    std::chrono::duration<double> diff = left.timestamp - right.timestamp;

    double p = left.pdu - right.pdu;
    double c = left.cpu - right.cpu;
    double g = left.gpu - right.gpu;
    double r = left.ram - right.ram;

    return vjoule::consumption_diff_t (diff.count (), p, c, g, r);
}

vjoule::consumption_diff_t operator+ (const vjoule::consumption_diff_t & left, const vjoule::consumption_diff_t & right) {
    float t = left.duration + right.duration;
    double p = left.pdu + right.pdu;
    double c = left.cpu + right.cpu;
    double g = left.gpu + right.gpu;
    double r = left.ram + right.ram;

    return vjoule::consumption_diff_t (t, p, c, g, r);
}

vjoule::consumption_diff_t operator- (const vjoule::consumption_diff_t & left, const vjoule::consumption_diff_t & right) {
    float t = left.duration - right.duration;
    double p = left.pdu - right.pdu;
    double c = left.cpu - right.cpu;
    double g = left.gpu - right.gpu;
    double r = left.ram - right.ram;

    return vjoule::consumption_diff_t (t, p, c, g, r);
}


vjoule::consumption_perc_t operator% (const vjoule::consumption_diff_t & left, const vjoule::consumption_diff_t & right) {

    float t = left.duration / right.duration * 100;
    float p = left.pdu / right.pdu * 100;
    float c = left.cpu / right.cpu * 100;
    float g = left.gpu / right.gpu * 100;
    float r = left.ram / right.ram * 100;

    return vjoule::consumption_perc_t (t, p, c, g, r);
}
