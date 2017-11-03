#include <iostream>
#include <cstring>
#include <list>

#include "umdhmm-v1.02/nrutil.h"
#include "umdhmm-v1.02/hmm.h"

#include "CallHmm.h"

namespace {
void PrintHmm (const HMM& hmm, const int& T, const int* O) {
	std::cerr << "DEBUG: CallHmm::HmmAndViterbi" << std::endl;
	std::cerr << "N, M: " << hmm.N << ", " << hmm.M << std::endl;
	std::cerr << "A" << std::endl;
	for (int i = 0; i < hmm.N; ++i) {
		std::cerr << "A" << i << std::endl;
		for (int j = 0; j < hmm.N; ++j)
			std::cerr << hmm.A[i][j] << " ";
		std::cerr << std::endl;
	}

	std::cerr << "B" << std::endl;
	for (int i = 0; i < hmm.N; ++i) {
		std::cerr << "B" << i << std::endl;
		for (int j = 0; j < hmm.M; ++j)
			std::cerr << hmm.B[i][j] << " ";
		std::cerr << std::endl;
	}

	std::cerr << "pi" << std::endl;
	for (int i = 0; i < hmm.N; ++i) {
		std::cerr << hmm.pi[i] << " ";
	}

	std::cerr << std::endl;
	std::cerr << "T: " << T << std::endl;
	std::cerr << "O" << std::endl;
	for (int i = 0; i < T; ++i) {
		std::cerr << O[i] << " ";
	}
	std::cerr << std::endl;
}

void SmoothStats(const std::list <SReadDepth>& read_depth, const int* q, const int T) {
	struct hmm_stats {
		hmm_stats(const unsigned int a, const unsigned int b, const unsigned int c): pos(a), stats(b), length(c){};
		unsigned int pos;
		unsigned int stats;
		unsigned int length;
	};
	std::list <hmm_stats> result;
	std::list <SReadDepth>::const_iterator rd_ite = read_depth.begin();
	for (int i = 1; i <= T; ++i, ++rd_ite) {
		if (result.empty() || q[i] != result.back().stats) { // Create the init hmm_stats.
			hmm_stats tmp(rd_ite->pos, q[i], 0);
			result.push_back(tmp);
		}
		++result.back().length;
	}

	for (std::list <hmm_stats>::const_iterator ite = result.begin(); ite != result.end(); ++ite) {
		std::cout << ite->pos << "\t" << ite->stats << "\t" << ite->length << std::endl;
	}

	std::list <hmm_stats> smooth_result;
	smooth_result.push_back(result.front());
	for (std::list <hmm_stats>::const_iterator ite = std::next(result.begin()); ite != result.end(); ++ite) {
		// If the current stats too small or the same as previous, then merge the cur to the previous stats.
		if ((ite->length / static_cast<double> ((std::prev(ite))->length)) < 0.1
			|| ite->stats == smooth_result.back().stats) { // merge
			smooth_result.back().length += ite->length;
		} else {
			smooth_result.push_back(*ite);
		}
	}

	std::cout << "After smoothing" << std::endl;
	for (std::list <hmm_stats>::const_iterator ite = smooth_result.begin(); ite != smooth_result.end(); ++ite) {
		std::cout << ite->pos << "\t" << ite->stats << "\t" << ite->length << std::endl;
	}
}
} // namespace

namespace CallHmm { 
bool HmmAndViterbi (const std::list <SReadDepth>& read_depth) {
	if (read_depth.empty()) return false;

	// Init HMM
	int T = read_depth.size();
	int* O = new int [T + 1]; // observation sequence O[1..T]
	for (std::list <SReadDepth>::const_iterator ite = read_depth.begin(); ite != read_depth.end(); ++ite) {
		O[std::distance(read_depth.begin(), ite) + 1] = ite->count;
	}

	HMM hmm;
	hmm.N  = hmm_N;
	hmm.M  = hmm_M;
	hmm.A  = new double* [hmm.N + 1];
	hmm.B  = new double* [hmm.N + 1];
	hmm.pi = new double [hmm.N + 1];

	for (int i = 1; i <= hmm.N; ++i) {
		hmm.A[i] = new double [hmm.N + 1];
		hmm.B[i] = new double [hmm.M + 1];
		std::memcpy(hmm.A[i] + 1, hmm_A[i - 1], sizeof(double)* hmm.N);
		std::memcpy(hmm.B[i] + 1, hmm_B[i - 1], sizeof(double) * hmm.M);
	}
	std::memcpy(hmm.pi + 1, hmm_pi, sizeof(double) * hmm.N);

	int* q = new int [T + 1]; // resultant states
	int** psi = new int* [T + 1];
	double **delta = new double* [T + 1];
	for (int i = 1; i <= T; ++i) {
		psi[i] = new int [hmm.N + 1];
		delta[i] = new double [hmm.N + 1];
	}
	double logproba = 0;
	// End of Init HMM
	
#ifdef DEBUG
PrintHmm(hmm, T, O);
#endif
	
	ViterbiLog(&hmm, T, O, delta, psi, q, &logproba);
	SmoothStats(read_depth, q, T);

	// Clean up
	for (int i = 1; i <= hmm.N; ++i) {
		delete hmm.A[i];
		delete hmm.B[i];
	}
	
	for (int i = 1; i <= T; ++i) {
		delete psi[i];
		delete delta[i];
	}
	
	delete O;
	delete hmm.A;
	delete hmm.B;
	delete hmm.pi;
	delete q;
	delete psi;
	delete delta;
	return true;
}
} // namespace CallHmm