while inotifywait /etc/vjoule/results/cpu; do
    glob=$(cat /etc/vjoule/results/cpu)
    vj=$(cat /etc/vjoule/results/vjoule.slice/vjoule_service.service/cpu)
    echo "Global consumption : $glob"
    echo "vJoule service consumption : $vj"
done
