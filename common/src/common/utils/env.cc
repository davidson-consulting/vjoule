#include <common/utils/env.hh>
#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <security/pam_appl.h>

#include <security/pam_misc.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>


namespace common::utils {

    void becomeSudo () {
	setuid (0);
    }

    struct pam_response *reply;

    //function used to get user input
    int function_conversation(int num_msg, const struct pam_message **msg, struct pam_response **resp, void *appdata_ptr)
    {
	*resp = reply;
	return PAM_SUCCESS;
    }
    
    std::string getUserName () {
	auto uid = geteuid ();
	auto pw = getpwuid (uid);
	if (pw != nullptr) {
	    return pw-> pw_name;
	}
	
	return "";
    }    

    void checkSudoers (const std::string & name) {
	int ngroups = 0;
	struct passwd *pw = getpwnam(name.c_str ());

	if (pw == nullptr) {
	    std::cerr <<  "getpwnam" << std::endl;
	    exit (-1);
	}

	getgrouplist(name.c_str (), pw->pw_gid, nullptr, &ngroups);
	
	gid_t *groups = (gid_t*) malloc(sizeof(gid_t) * ngroups);	
	getgrouplist(name.c_str (), pw->pw_gid, groups, &ngroups);
	
	for (int j = 0; j < ngroups; j++) {
	    auto gr = getgrgid(groups[j]);
	    if (strcmp (gr-> gr_name , "sudo") == 0) {
		free (groups);
		return ;
	    }
	}

	free (groups);
	std::cerr << "user '" << name << "' is not in sudo group" << std::endl;
	exit (-1);
    }
    
    void authenticateSudo () {
	auto user = std::string (getenv ("USER"));
	checkSudoers (user);

	const struct pam_conv local_conversation = { &function_conversation, NULL };
	pam_handle_t *local_auth_handle = NULL; // this gets set by pam_start

	int retval;

	// local_auth_handle gets set based on the service
	retval = pam_start("common-auth", user.c_str (), &local_conversation, &local_auth_handle);

	if (retval != PAM_SUCCESS)
	{
	    std::cout << "pam_start returned " << retval << std::endl;
	    exit(retval);
	}

	reply = (struct pam_response *)malloc(sizeof(struct pam_response));

	// *** Get the password by any method, or maybe it was passed into this function.
	reply[0].resp = getpass ((std::string ("[sudo] password for ") + user + ": ").c_str ());
	reply[0].resp_retcode = 0;

	retval = pam_authenticate(local_auth_handle, 0);

	if (retval != PAM_SUCCESS)
	{
	    if (retval == PAM_AUTH_ERR)
	    {
		std::cout << "Authentication failure." << std::endl;
	    }
	    else
	    {
		std::cout << "pam_authenticate returned " << retval << std::endl;
	    }
	    
	    exit(retval);
	}

	retval = pam_end(local_auth_handle, retval);

	if (retval != PAM_SUCCESS)
	{
	    std::cout << "pam_end returned " << retval << std::endl;
	    exit(retval);
	}
    }

    
}
