
// MULTIPLE STATIC WAVESHAPING CLASSES
// THESE CLASSES INCORPORATE THE ANTIDERIVATE ANTIALIASING METHOD DESCRIBED IN:
// "ANTIDERIVATIVE ANTIALIASING FOR MEMORYLESS NONLINEARITIES" BY S. BILBAO ET AL. IEEE SPL, 2017
//
// THIS CODE IS PROVIDED "AS-IS", WITH NO GUARANTEE OF ANY KIND.
// 
// CODED BY F. ESQUEDA - DECEMBER 2017
// 
// ADAPTED FOR VCV RACK JANUARY 2018
// 
// TODO: 
// 		ADD MORE STANDARD FILTERS
#ifndef WAVESHAPING_H
#define WAVESHAPING_H 

class HardClipper {

// THIS CLASS IMPLEMENTS AN ANTIALIASED HARD CLIPPING FUNCTION.
// THIS CLASS USES THE FIRST-ORDER ANTIDERIVATIVE METHOD.

private:

	float output = 0.0;

	float xn1 = 0.0;
	float Fn = 0.0;
	float Fn1 = 0.0;

	const float thresh = 10.0e-2;
	const float oneTwelfth = 1.0/12.0;

public:

	HardClipper() {}
	~HardClipper() {}

	void process(float input) {
		output = antialiasedHardClipN1(input);
	}

	float signum(float x) {
		return (x > 0.0f) ? 1.0f : ((x < 0.0f) ? -1.0f : 0.0f);
	}

	float hardClipN0(float x) {
		// Hard clipping function
		return 0.5f*(signum(x+1.0f)*(x+1.0f) - signum(x-1.0f)*(x-1.0f)); 
	}

	float hardClipN1(float x) {
		// First antiderivative of hardClipN0
		return 0.25f*(signum(x+1.0f)*(x+1.0f)*(x+1.0f) - signum(x-1.0f)*(x-1.0f)*(x-1.0f) - 2.0f); 
	}

	float hardClipN2(float x) {
		// second antiderivative of hardClipN0
		return oneTwelfth*(signum(x+1.0f)*(x+1.0f)*(x+1.0f)*(x+1.0f) - signum(x-1.0f)*(x-1.0f)*(x-1.0f)*(x-1.0f) - 6.0f*x); 
	}

	float antialiasedHardClipN1(float x) {

		// Hard clipping with 1st-order antialiasing
		Fn = hardClipN1(x);
		float tmp = 0.0;
		if (std::abs(x - xn1) < thresh) {
			tmp = hardClipN0(0.5f * (x + xn1));
		}
		else {
			tmp = (Fn - Fn1)/(x - xn1);
		}

		// Update states
		xn1 = x;
		Fn1 = Fn;

		return tmp;
	}

	float getClippedOutput() {
		return output;
	}

};

class Wavefolder {

// THIS CLASS IMPLEMENTS A FOLDING FUNCTION, SOMETIMES KNOWN AS A MATHEMATICAL FOLDER DUE TO ITS
// SHARP EDGES. THE SOUND PRODUCED IS SIMILAR TO THAT OF THE BUCHLA 259'S TIMBRE SECTION. 

private:

	float output = 0.0;

	// Antialiasing state variables
	float xn1 = 0.0;
	float xn2 = 0.0;
	float Fn = 0.0;
	float Fn1 = 0.0;
	float Gn = 0.0;
	float Gn1 = 0.0;

	// Ill-conditioning threshold
	const float thresh = 10.0e-2;

	const float oneSixth = 1.0/6.0;

	HardClipper hardClipper;

public:

	Wavefolder() {}
	~Wavefolder() {}

	void process(float input) {
		output = antialiasedFoldN2(input);
	}

	float foldFunctionN0(float x) {
		// Folding function
		return (2.0f*hardClipper.hardClipN0(x) - x);
	}

	float foldFunctionN1(float x) {	
		// First antiderivative of the folding function
		return (2.0f*hardClipper.hardClipN1(x) - 0.5f*x*x);
	}

	float foldFunctionN2(float x) {	
		// Second antiderivative of the folding function
		return (2.0f*hardClipper.hardClipN2(x) - oneSixth*(x*x*x));
	}

	float antialiasedFoldN1(float x) {

		// Folding with 1st-order antialiasing (not recommended)
		Fn = foldFunctionN1(x);
		float tmp = 0.0;
		if (std::abs(x - xn1) < thresh) {
			tmp = foldFunctionN0(0.5f * (x + xn1));
		}
		else {
			tmp = (Fn - Fn1)/(x - xn1);
		}

		// Update states
		xn1 = x;
		Fn1 = Fn;

		return tmp;
	}

	float antialiasedFoldN2(float x) {

		// Folding with 2nd-order antialiasing
		Fn = foldFunctionN2(x);
		float tmp = 0.0;
		if (std::abs(x - xn1) < thresh) {
			// First-order escape rule
			Gn = foldFunctionN1(0.5f * (x + xn1));
		}
		else {
			Gn = (Fn - Fn1) / (x - xn1);
		}

		if (std::abs(x - xn2) < thresh) {
			// Second-order escape
			float delta = 0.5f * (x - 2.0f*xn1 + xn2);
			if (std::abs(delta) < thresh) {
				tmp = foldFunctionN0(0.25f * (x + 2.0f*xn1 + xn2));
			}
			else {
				float tmp1 = foldFunctionN1(0.5f * (x + xn2));
				float tmp2 = foldFunctionN2(0.5f * (x + xn2));
				tmp = (2.0f/delta)*(tmp1 + (Fn1 - tmp2)/delta);
			}
		}
		else {
			tmp = 2.0f * (Gn - Gn1)/(x - xn2);
		}

		 // Update state variables
		Fn1 = Fn;
		Gn1 = Gn;
		xn2 = xn1;
		xn1 = x;

		return tmp;
	}

	float getFoldedOutput() {
		return output;
	}

};

class SoftClipper {

// THIS CLASS IMPLEMENTS A PIECEWISE SOFT SATURATOR WITH 
// FIRST-ORDER ANTIDERIVATIVE ANTIALIASING.

private:

	float output = 0.0;

	// Antialiasing variables
	float xn1 = 0.0;
	float Fn = 0.0;
	float Fn1 = 0.0;
	const float thresh = 10.0e-2;

public:
	SoftClipper() {}
	~SoftClipper() {}

	void process(float input) {
		output = antialiasedSoftClipN1(input);
	}

	float signum(float x) {
		return (x > 0.0f) ? 1.0f : ((x < 0.0f) ? -1.0f : 0.0f);
	}

	float softClipN0(float x) {
		return (std::abs(x)<1) ? std::sin(0.5f*M_PI*x) : signum(x); 
	}

	float softClipN1(float x) {
		
		return (std::abs(x)<1) ? 1.0f - (2.0f/M_PI)*cos(x*0.5f*M_PI) : signum(x)*x;
	}

	float antialiasedSoftClipN1(float x) {

		Fn = softClipN1(x);
		float tmp = 0.0;
		if (std::abs(x - xn1) < thresh) {
			tmp = softClipN0(0.5f * (x + xn1));
		}
		else {
			tmp = (Fn - Fn1)/(x - xn1);
		}

		// Update states
		xn1 = x;
		Fn1 = Fn;

		return tmp;
	}

	float getClippedOutput() {
		return output;
	}
};

#endif 

// EOF