#include <iostream>

#include <vjoule.hh>

// Ajouter la possibilité de demander au sensor de dumper à la commande
// On aurait un sensor avec une fréquence de 0 (ne fait rien)
// On lui demanderait de dumper à la commande
int main(int argc, char * argv[]) {
	tools::vjoule::VJoule vjoule(argc, argv);
	vjoule.run();

	return 0;	
}
