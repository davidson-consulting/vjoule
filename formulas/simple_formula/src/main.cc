#include <common/formula/formula.hh>
#include <simple_formula/divider.hh>

int main (int argc, char ** argv) {
    common::formula::Formula<simple_formula::Divider>  formula ("simple_formula", argc, argv);
    formula.run ();
}
