#include "Rcpp.h"

#include "hash_sequence.h"
#include "build_dictionary.h"
#include "search_sequence.h"

#include <stdexcept>
#include <vector>
#include <sstream>
#include <map>

/* Combination guide parser. */

template<size_t nvariable>
struct combo_store {
    struct combo_comp {
        bool operator()(const combination<nvariable>& left,
                const combination<nvariable>& right) const 
        {
            for (size_t i=0; i<nvariable; ++i) {
                if (left.indices[i] < right.indices[i]) { return true; }
                if (left.indices[i] > right.indices[i]) { return false; }
            }
            return false;
        }
    };

    std::map<combination<nvariable>, int, combo_comp> store;

    void operator()(const combination<nvariable>& input) {
        // Int value in 'store' value-initializes to zero, so this is valid.
        ++(store[input]);
        return;
    }
};

template<size_t nvariable> 
struct se_combo_info {
    se_combo_info(Rcpp::List g, Rcpp::StringVector c, Rcpp::LogicalVector s, Rcpp::LogicalVector d) :
        info(g, c, s, d) {}
    search_info<nvariable> info;
    combo_store<nvariable> output;
};

template<size_t nvariable>
SEXP setup_barcodes_combo(SEXP constants, SEXP guide_list, Rcpp::LogicalVector allowS, Rcpp::LogicalVector allowD) 
{
    se_combo_info<nvariable> * ptr = new se_combo_info<nvariable>(
        guide_list, constants, allowS, allowD
    );
    return Rcpp::XPtr<se_combo_info<nvariable> >(ptr, true);
}

template<size_t nvariable>
SEXP count_barcodes_combo(SEXP seqs, SEXP xptr) {
    Rcpp::XPtr<se_combo_info<nvariable> > ptr(xptr);
    const auto& search_info=ptr->info;
    auto& output=ptr->output;

    Rcpp::StringVector Seqs(seqs);
    for (size_t i=0; i<Seqs.size(); ++i) {
        Rcpp::String s=Seqs[i];
        const char* sptr=s.get_cstring();
        const size_t len=Rf_length(s.get_sexp());
        search_sequence(sptr, len, search_info, output);
    }

    return R_NilValue;
}

template<size_t nvariable>
SEXP report_barcodes_combo(SEXP xptr) {
    Rcpp::XPtr<se_combo_info<nvariable> > ptr(xptr);
    const auto& out_store=ptr->output;

    const size_t ncombos=out_store.store.size();
    std::vector<Rcpp::IntegerVector> keys(nvariable);
    for (auto& k : keys) { k = Rcpp::IntegerVector(ncombos); }
    Rcpp::IntegerVector counts(ncombos);

    size_t i=0;
    for (const auto& pairing : out_store.store) {
        const auto& key=pairing.first;
        for (size_t j=0; j<nvariable; ++j) {
            keys[j][i]=key.indices[j]+1; // get back to 1-based indexing.
        }
        counts[i]=pairing.second; 
        ++i;
    }

    Rcpp::List output(2);
    output[0]=counts;
    output[1]=Rcpp::List(keys.begin(), keys.end());
    return output;
}

/****************************************************
 * Realizations of template functions for 2 guides. *
 ****************************************************/

// [[Rcpp::export(rng=false)]]
SEXP setup_barcodes_combo_dual(SEXP constants, SEXP guide_list, Rcpp::LogicalVector allowSub, Rcpp::LogicalVector allowDel) {
    return setup_barcodes_combo<2>(constants, guide_list, allowSub, allowDel);
}

// [[Rcpp::export(rng=false)]]
SEXP count_barcodes_combo_dual(SEXP seqs, SEXP xptr) {
    return count_barcodes_combo<2>(seqs, xptr);
}

// [[Rcpp::export(rng=false)]]
SEXP report_barcodes_combo_dual(SEXP xptr) {
    return report_barcodes_combo<2>(xptr);
}