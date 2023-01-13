#include <common/formula/formula.hh>
#include <divider.hh>

int main (int argc, char ** argv) {
    common::formula::Formula<dumping_formula::Divider>  formula ("dumping_formula", argc, argv);
    formula.run ();
}
