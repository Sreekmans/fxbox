/* ------------------------------------------------------------
license: "GPL-3.0-or-later"
name: "fxbox_reverb"
Code generated with Faust 2.70.3 (https://faust.grame.fr)
Compilation options: -a arch_minimal.cpp -lang cpp -ct 1 -cn ReverbDsp -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0
------------------------------------------------------------ */

#ifndef  __ReverbDsp_H__
#define  __ReverbDsp_H__

/*
 * Minimal Faust architecture for FXBox.
 * Produces a self-contained C++ class (no Faust runtime headers needed on the Pi).
 * The generated class is wrapped in a namespace by the build script.
 */
#pragma once
#include <cmath>
#include <cstdint>
#include <cstring>
#include <algorithm>

#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif

#ifndef FXBOX_FAUST_BASE
#define FXBOX_FAUST_BASE
namespace fxfaust {

struct Meta {
    virtual ~Meta() {}
    virtual void declare(const char*, const char*) {}
};

struct UI {
    virtual ~UI() {}
    virtual void openTabBox(const char*) {}
    virtual void openHorizontalBox(const char*) {}
    virtual void openVerticalBox(const char*) {}
    virtual void closeBox() {}
    virtual void addButton(const char* label, FAUSTFLOAT* zone) = 0;
    virtual void addCheckButton(const char* label, FAUSTFLOAT* zone) = 0;
    virtual void addVerticalSlider(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step) = 0;
    virtual void addHorizontalSlider(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step) = 0;
    virtual void addNumEntry(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step) = 0;
    virtual void addHorizontalBargraph(const char*, FAUSTFLOAT*, FAUSTFLOAT, FAUSTFLOAT) {}
    virtual void addVerticalBargraph(const char*, FAUSTFLOAT*, FAUSTFLOAT, FAUSTFLOAT) {}
    virtual void addSoundfile(const char*, const char*, void**) {}
    virtual void declare(FAUSTFLOAT*, const char*, const char*) {}
};

struct dsp {
    virtual ~dsp() {}
    virtual int getNumInputs() = 0;
    virtual int getNumOutputs() = 0;
    virtual void buildUserInterface(UI* ui_interface) = 0;
    virtual int getSampleRate() = 0;
    virtual void init(int sample_rate) = 0;
    virtual void instanceInit(int sample_rate) = 0;
    virtual void instanceConstants(int sample_rate) = 0;
    virtual void instanceResetUserInterface() = 0;
    virtual void instanceClear() = 0;
    virtual dsp* clone() = 0;
    virtual void metadata(Meta* m) = 0;
    virtual void compute(int count, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs) = 0;
};

} // namespace fxfaust
#endif

namespace fxfaust {
#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif 

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <math.h>

#ifndef FAUSTCLASS 
#define FAUSTCLASS ReverbDsp
#endif

#ifdef __APPLE__ 
#define exp10f __exp10f
#define exp10 __exp10
#endif

#if defined(_WIN32)
#define RESTRICT __restrict
#else
#define RESTRICT __restrict__
#endif

static float ReverbDsp_faustpower2_f(float value) {
	return value * value;
}

class ReverbDsp : public dsp {
	
 private:
	
	int fSampleRate;
	float fConst1;
	float fConst2;
	FAUSTFLOAT fHslider0;
	float fRec1[2];
	float fConst3;
	FAUSTFLOAT fHslider1;
	float fRec13[2];
	float fConst6;
	float fConst7;
	float fConst9;
	float fConst10;
	float fRec14[2];
	float fRec12[2];
	int IOTA0;
	float fVec0[16384];
	int iConst12;
	float fVec1[32768];
	FAUSTFLOAT fHslider2;
	float fRec15[2];
	float fConst13;
	float fVec2[4096];
	int iConst14;
	float fRec10[2];
	float fConst17;
	float fConst18;
	float fRec19[2];
	float fRec18[2];
	float fVec3[16384];
	int iConst20;
	float fVec4[2048];
	int iConst21;
	float fRec16[2];
	float fConst24;
	float fConst25;
	float fRec23[2];
	float fRec22[2];
	float fVec5[16384];
	int iConst27;
	float fVec6[4096];
	int iConst28;
	float fRec20[2];
	float fConst31;
	float fConst32;
	float fRec27[2];
	float fRec26[2];
	float fVec7[16384];
	int iConst34;
	float fVec8[2048];
	int iConst35;
	float fRec24[2];
	float fConst38;
	float fConst39;
	float fRec31[2];
	float fRec30[2];
	float fVec9[32768];
	int iConst41;
	float fVec10[32768];
	float fVec11[4096];
	int iConst42;
	float fRec28[2];
	float fConst45;
	float fConst46;
	float fRec35[2];
	float fRec34[2];
	float fVec12[16384];
	int iConst48;
	float fVec13[4096];
	int iConst49;
	float fRec32[2];
	float fConst52;
	float fConst53;
	float fRec39[2];
	float fRec38[2];
	float fVec14[32768];
	int iConst55;
	float fVec15[4096];
	int iConst56;
	float fRec36[2];
	float fConst59;
	float fConst60;
	float fRec43[2];
	float fRec42[2];
	float fVec16[32768];
	int iConst62;
	float fVec17[2048];
	int iConst63;
	float fRec40[2];
	float fRec2[3];
	float fRec3[3];
	float fRec4[3];
	float fRec5[3];
	float fRec6[3];
	float fRec7[3];
	float fRec8[3];
	float fRec9[3];
	float fVec18[2];
	float fRec0[2];
	float fVec19[2];
	float fRec44[2];
	
 public:
	ReverbDsp() {}

	void metadata(Meta* m) { 
		m->declare("basics.lib/name", "Faust Basic Element Library");
		m->declare("basics.lib/tabulateNd", "Copyright (C) 2023 Bart Brouns <bart@magnetophon.nl>");
		m->declare("basics.lib/version", "1.12.0");
		m->declare("compile_options", "-a arch_minimal.cpp -lang cpp -ct 1 -cn ReverbDsp -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0");
		m->declare("delays.lib/name", "Faust Delay Library");
		m->declare("delays.lib/version", "1.1.0");
		m->declare("filename", "reverb.dsp");
		m->declare("filters.lib/allpass_comb:author", "Julius O. Smith III");
		m->declare("filters.lib/allpass_comb:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/allpass_comb:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/lowpass0_highpass1", "MIT-style STK-4.3 license");
		m->declare("filters.lib/lowpass0_highpass1:author", "Julius O. Smith III");
		m->declare("filters.lib/lowpass:author", "Julius O. Smith III");
		m->declare("filters.lib/lowpass:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/lowpass:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/name", "Faust Filters Library");
		m->declare("filters.lib/tf1:author", "Julius O. Smith III");
		m->declare("filters.lib/tf1:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/tf1:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/tf1s:author", "Julius O. Smith III");
		m->declare("filters.lib/tf1s:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/tf1s:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/version", "1.3.0");
		m->declare("license", "GPL-3.0-or-later");
		m->declare("maths.lib/author", "GRAME");
		m->declare("maths.lib/copyright", "GRAME");
		m->declare("maths.lib/license", "LGPL with exception");
		m->declare("maths.lib/name", "Faust Math Library");
		m->declare("maths.lib/version", "2.7.0");
		m->declare("name", "fxbox_reverb");
		m->declare("platform.lib/name", "Generic Platform Library");
		m->declare("platform.lib/version", "1.3.0");
		m->declare("reverbs.lib/name", "Faust Reverb Library");
		m->declare("reverbs.lib/version", "1.2.1");
		m->declare("routes.lib/hadamard:author", "Remy Muller, revised by Romain Michon");
		m->declare("routes.lib/name", "Faust Signal Routing Library");
		m->declare("routes.lib/version", "1.2.0");
		m->declare("signals.lib/name", "Faust Signal Routing Library");
		m->declare("signals.lib/version", "1.5.0");
	}

	virtual int getNumInputs() {
		return 2;
	}
	virtual int getNumOutputs() {
		return 2;
	}
	
	static void classInit(int sample_rate) {
	}
	
	virtual void instanceConstants(int sample_rate) {
		fSampleRate = sample_rate;
		float fConst0 = std::min<float>(1.92e+05f, std::max<float>(1.0f, float(fSampleRate)));
		fConst1 = 44.1f / fConst0;
		fConst2 = 1.0f - fConst1;
		fConst3 = 9424.778f / fConst0;
		float fConst4 = std::floor(0.174713f * fConst0 + 0.5f);
		float fConst5 = fConst4 / fConst0;
		fConst6 = 6.9077554f * fConst5;
		fConst7 = 6.006744f * fConst5;
		float fConst8 = 1.0f / std::tan(628.31854f / fConst0);
		fConst9 = 1.0f - fConst8;
		fConst10 = 1.0f / (fConst8 + 1.0f);
		float fConst11 = std::floor(0.022904f * fConst0 + 0.5f);
		iConst12 = int(std::min<float>(8192.0f, std::max<float>(0.0f, fConst4 - fConst11)));
		fConst13 = 0.001f * fConst0;
		iConst14 = int(std::min<float>(2048.0f, std::max<float>(0.0f, fConst11 + -1.0f)));
		float fConst15 = std::floor(0.153129f * fConst0 + 0.5f);
		float fConst16 = fConst15 / fConst0;
		fConst17 = 6.9077554f * fConst16;
		fConst18 = 6.006744f * fConst16;
		float fConst19 = std::floor(0.020346f * fConst0 + 0.5f);
		iConst20 = int(std::min<float>(8192.0f, std::max<float>(0.0f, fConst15 - fConst19)));
		iConst21 = int(std::min<float>(1024.0f, std::max<float>(0.0f, fConst19 + -1.0f)));
		float fConst22 = std::floor(0.127837f * fConst0 + 0.5f);
		float fConst23 = fConst22 / fConst0;
		fConst24 = 6.9077554f * fConst23;
		fConst25 = 6.006744f * fConst23;
		float fConst26 = std::floor(0.031604f * fConst0 + 0.5f);
		iConst27 = int(std::min<float>(8192.0f, std::max<float>(0.0f, fConst22 - fConst26)));
		iConst28 = int(std::min<float>(2048.0f, std::max<float>(0.0f, fConst26 + -1.0f)));
		float fConst29 = std::floor(0.125f * fConst0 + 0.5f);
		float fConst30 = fConst29 / fConst0;
		fConst31 = 6.9077554f * fConst30;
		fConst32 = 6.006744f * fConst30;
		float fConst33 = std::floor(0.013458f * fConst0 + 0.5f);
		iConst34 = int(std::min<float>(8192.0f, std::max<float>(0.0f, fConst29 - fConst33)));
		iConst35 = int(std::min<float>(1024.0f, std::max<float>(0.0f, fConst33 + -1.0f)));
		float fConst36 = std::floor(0.210389f * fConst0 + 0.5f);
		float fConst37 = fConst36 / fConst0;
		fConst38 = 6.9077554f * fConst37;
		fConst39 = 6.006744f * fConst37;
		float fConst40 = std::floor(0.024421f * fConst0 + 0.5f);
		iConst41 = int(std::min<float>(16384.0f, std::max<float>(0.0f, fConst36 - fConst40)));
		iConst42 = int(std::min<float>(2048.0f, std::max<float>(0.0f, fConst40 + -1.0f)));
		float fConst43 = std::floor(0.192303f * fConst0 + 0.5f);
		float fConst44 = fConst43 / fConst0;
		fConst45 = 6.9077554f * fConst44;
		fConst46 = 6.006744f * fConst44;
		float fConst47 = std::floor(0.029291f * fConst0 + 0.5f);
		iConst48 = int(std::min<float>(8192.0f, std::max<float>(0.0f, fConst43 - fConst47)));
		iConst49 = int(std::min<float>(2048.0f, std::max<float>(0.0f, fConst47 + -1.0f)));
		float fConst50 = std::floor(0.256891f * fConst0 + 0.5f);
		float fConst51 = fConst50 / fConst0;
		fConst52 = 6.9077554f * fConst51;
		fConst53 = 6.006744f * fConst51;
		float fConst54 = std::floor(0.027333f * fConst0 + 0.5f);
		iConst55 = int(std::min<float>(16384.0f, std::max<float>(0.0f, fConst50 - fConst54)));
		iConst56 = int(std::min<float>(2048.0f, std::max<float>(0.0f, fConst54 + -1.0f)));
		float fConst57 = std::floor(0.219991f * fConst0 + 0.5f);
		float fConst58 = fConst57 / fConst0;
		fConst59 = 6.9077554f * fConst58;
		fConst60 = 6.006744f * fConst58;
		float fConst61 = std::floor(0.019123f * fConst0 + 0.5f);
		iConst62 = int(std::min<float>(16384.0f, std::max<float>(0.0f, fConst57 - fConst61)));
		iConst63 = int(std::min<float>(1024.0f, std::max<float>(0.0f, fConst61 + -1.0f)));
	}
	
	virtual void instanceResetUserInterface() {
		fHslider0 = FAUSTFLOAT(0.5f);
		fHslider1 = FAUSTFLOAT(2.5f);
		fHslider2 = FAUSTFLOAT(2e+01f);
	}
	
	virtual void instanceClear() {
		for (int l0 = 0; l0 < 2; l0 = l0 + 1) {
			fRec1[l0] = 0.0f;
		}
		for (int l1 = 0; l1 < 2; l1 = l1 + 1) {
			fRec13[l1] = 0.0f;
		}
		for (int l2 = 0; l2 < 2; l2 = l2 + 1) {
			fRec14[l2] = 0.0f;
		}
		for (int l3 = 0; l3 < 2; l3 = l3 + 1) {
			fRec12[l3] = 0.0f;
		}
		IOTA0 = 0;
		for (int l4 = 0; l4 < 16384; l4 = l4 + 1) {
			fVec0[l4] = 0.0f;
		}
		for (int l5 = 0; l5 < 32768; l5 = l5 + 1) {
			fVec1[l5] = 0.0f;
		}
		for (int l6 = 0; l6 < 2; l6 = l6 + 1) {
			fRec15[l6] = 0.0f;
		}
		for (int l7 = 0; l7 < 4096; l7 = l7 + 1) {
			fVec2[l7] = 0.0f;
		}
		for (int l8 = 0; l8 < 2; l8 = l8 + 1) {
			fRec10[l8] = 0.0f;
		}
		for (int l9 = 0; l9 < 2; l9 = l9 + 1) {
			fRec19[l9] = 0.0f;
		}
		for (int l10 = 0; l10 < 2; l10 = l10 + 1) {
			fRec18[l10] = 0.0f;
		}
		for (int l11 = 0; l11 < 16384; l11 = l11 + 1) {
			fVec3[l11] = 0.0f;
		}
		for (int l12 = 0; l12 < 2048; l12 = l12 + 1) {
			fVec4[l12] = 0.0f;
		}
		for (int l13 = 0; l13 < 2; l13 = l13 + 1) {
			fRec16[l13] = 0.0f;
		}
		for (int l14 = 0; l14 < 2; l14 = l14 + 1) {
			fRec23[l14] = 0.0f;
		}
		for (int l15 = 0; l15 < 2; l15 = l15 + 1) {
			fRec22[l15] = 0.0f;
		}
		for (int l16 = 0; l16 < 16384; l16 = l16 + 1) {
			fVec5[l16] = 0.0f;
		}
		for (int l17 = 0; l17 < 4096; l17 = l17 + 1) {
			fVec6[l17] = 0.0f;
		}
		for (int l18 = 0; l18 < 2; l18 = l18 + 1) {
			fRec20[l18] = 0.0f;
		}
		for (int l19 = 0; l19 < 2; l19 = l19 + 1) {
			fRec27[l19] = 0.0f;
		}
		for (int l20 = 0; l20 < 2; l20 = l20 + 1) {
			fRec26[l20] = 0.0f;
		}
		for (int l21 = 0; l21 < 16384; l21 = l21 + 1) {
			fVec7[l21] = 0.0f;
		}
		for (int l22 = 0; l22 < 2048; l22 = l22 + 1) {
			fVec8[l22] = 0.0f;
		}
		for (int l23 = 0; l23 < 2; l23 = l23 + 1) {
			fRec24[l23] = 0.0f;
		}
		for (int l24 = 0; l24 < 2; l24 = l24 + 1) {
			fRec31[l24] = 0.0f;
		}
		for (int l25 = 0; l25 < 2; l25 = l25 + 1) {
			fRec30[l25] = 0.0f;
		}
		for (int l26 = 0; l26 < 32768; l26 = l26 + 1) {
			fVec9[l26] = 0.0f;
		}
		for (int l27 = 0; l27 < 32768; l27 = l27 + 1) {
			fVec10[l27] = 0.0f;
		}
		for (int l28 = 0; l28 < 4096; l28 = l28 + 1) {
			fVec11[l28] = 0.0f;
		}
		for (int l29 = 0; l29 < 2; l29 = l29 + 1) {
			fRec28[l29] = 0.0f;
		}
		for (int l30 = 0; l30 < 2; l30 = l30 + 1) {
			fRec35[l30] = 0.0f;
		}
		for (int l31 = 0; l31 < 2; l31 = l31 + 1) {
			fRec34[l31] = 0.0f;
		}
		for (int l32 = 0; l32 < 16384; l32 = l32 + 1) {
			fVec12[l32] = 0.0f;
		}
		for (int l33 = 0; l33 < 4096; l33 = l33 + 1) {
			fVec13[l33] = 0.0f;
		}
		for (int l34 = 0; l34 < 2; l34 = l34 + 1) {
			fRec32[l34] = 0.0f;
		}
		for (int l35 = 0; l35 < 2; l35 = l35 + 1) {
			fRec39[l35] = 0.0f;
		}
		for (int l36 = 0; l36 < 2; l36 = l36 + 1) {
			fRec38[l36] = 0.0f;
		}
		for (int l37 = 0; l37 < 32768; l37 = l37 + 1) {
			fVec14[l37] = 0.0f;
		}
		for (int l38 = 0; l38 < 4096; l38 = l38 + 1) {
			fVec15[l38] = 0.0f;
		}
		for (int l39 = 0; l39 < 2; l39 = l39 + 1) {
			fRec36[l39] = 0.0f;
		}
		for (int l40 = 0; l40 < 2; l40 = l40 + 1) {
			fRec43[l40] = 0.0f;
		}
		for (int l41 = 0; l41 < 2; l41 = l41 + 1) {
			fRec42[l41] = 0.0f;
		}
		for (int l42 = 0; l42 < 32768; l42 = l42 + 1) {
			fVec16[l42] = 0.0f;
		}
		for (int l43 = 0; l43 < 2048; l43 = l43 + 1) {
			fVec17[l43] = 0.0f;
		}
		for (int l44 = 0; l44 < 2; l44 = l44 + 1) {
			fRec40[l44] = 0.0f;
		}
		for (int l45 = 0; l45 < 3; l45 = l45 + 1) {
			fRec2[l45] = 0.0f;
		}
		for (int l46 = 0; l46 < 3; l46 = l46 + 1) {
			fRec3[l46] = 0.0f;
		}
		for (int l47 = 0; l47 < 3; l47 = l47 + 1) {
			fRec4[l47] = 0.0f;
		}
		for (int l48 = 0; l48 < 3; l48 = l48 + 1) {
			fRec5[l48] = 0.0f;
		}
		for (int l49 = 0; l49 < 3; l49 = l49 + 1) {
			fRec6[l49] = 0.0f;
		}
		for (int l50 = 0; l50 < 3; l50 = l50 + 1) {
			fRec7[l50] = 0.0f;
		}
		for (int l51 = 0; l51 < 3; l51 = l51 + 1) {
			fRec8[l51] = 0.0f;
		}
		for (int l52 = 0; l52 < 3; l52 = l52 + 1) {
			fRec9[l52] = 0.0f;
		}
		for (int l53 = 0; l53 < 2; l53 = l53 + 1) {
			fVec18[l53] = 0.0f;
		}
		for (int l54 = 0; l54 < 2; l54 = l54 + 1) {
			fRec0[l54] = 0.0f;
		}
		for (int l55 = 0; l55 < 2; l55 = l55 + 1) {
			fVec19[l55] = 0.0f;
		}
		for (int l56 = 0; l56 < 2; l56 = l56 + 1) {
			fRec44[l56] = 0.0f;
		}
	}
	
	virtual void init(int sample_rate) {
		classInit(sample_rate);
		instanceInit(sample_rate);
	}
	
	virtual void instanceInit(int sample_rate) {
		instanceConstants(sample_rate);
		instanceResetUserInterface();
		instanceClear();
	}
	
	virtual ReverbDsp* clone() {
		return new ReverbDsp();
	}
	
	virtual int getSampleRate() {
		return fSampleRate;
	}
	
	virtual void buildUserInterface(UI* ui_interface) {
		ui_interface->openVerticalBox("fxbox_reverb");
		ui_interface->addHorizontalSlider("decay", &fHslider1, FAUSTFLOAT(2.5f), FAUSTFLOAT(0.2f), FAUSTFLOAT(15.0f), FAUSTFLOAT(0.001f));
		ui_interface->addHorizontalSlider("predelay", &fHslider2, FAUSTFLOAT(2e+01f), FAUSTFLOAT(0.0f), FAUSTFLOAT(2.5e+02f), FAUSTFLOAT(0.01f));
		ui_interface->addHorizontalSlider("tone", &fHslider0, FAUSTFLOAT(0.5f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(0.0001f));
		ui_interface->closeBox();
	}
	
	virtual void compute(int count, FAUSTFLOAT** RESTRICT inputs, FAUSTFLOAT** RESTRICT outputs) {
		FAUSTFLOAT* input0 = inputs[0];
		FAUSTFLOAT* input1 = inputs[1];
		FAUSTFLOAT* output0 = outputs[0];
		FAUSTFLOAT* output1 = outputs[1];
		float fSlow0 = fConst1 * float(fHslider0);
		float fSlow1 = fConst1 * float(fHslider1);
		float fSlow2 = fConst1 * float(fHslider2);
		for (int i0 = 0; i0 < count; i0 = i0 + 1) {
			fRec1[0] = fSlow0 + fConst2 * fRec1[1];
			float fTemp0 = 1.0f / std::tan(fConst3 * std::pow(6.0f, fRec1[0]));
			float fTemp1 = fTemp0 + 1.0f;
			float fTemp2 = 1.0f - fTemp0;
			fRec13[0] = fSlow1 + fConst2 * fRec13[1];
			float fTemp3 = std::max<float>(0.2f, fRec13[0]);
			float fTemp4 = std::exp(-(fConst6 / fTemp3));
			float fTemp5 = ReverbDsp_faustpower2_f(fTemp4);
			float fTemp6 = 1.0f - fTemp5;
			float fTemp7 = std::cos(fConst3 * std::pow(8.0f, fRec1[0]));
			float fTemp8 = 1.0f - fTemp7 * fTemp5;
			float fTemp9 = std::sqrt(std::max<float>(0.0f, ReverbDsp_faustpower2_f(fTemp8) / ReverbDsp_faustpower2_f(fTemp6) + -1.0f));
			float fTemp10 = fTemp8 / fTemp6;
			fRec14[0] = -(fConst10 * (fConst9 * fRec14[1] - (fRec6[1] + fRec6[2])));
			fRec12[0] = fTemp4 * (fTemp9 + (1.0f - fTemp10)) * (fRec6[1] + fRec14[0] * (std::exp(-(fConst7 / fTemp3)) / fTemp4 + -1.0f)) + (fTemp10 - fTemp9) * fRec12[1];
			fVec0[IOTA0 & 16383] = 0.35355338f * fRec12[0] + 1e-20f;
			fVec1[IOTA0 & 32767] = float(input0[i0]);
			fRec15[0] = fSlow2 + fConst2 * fRec15[1];
			int iTemp11 = int(std::min<float>(16384.0f, std::max<float>(0.0f, fConst13 * fRec15[0])));
			float fTemp12 = 0.3f * fVec1[(IOTA0 - iTemp11) & 32767];
			float fTemp13 = fTemp12 + fVec0[(IOTA0 - iConst12) & 16383] - 0.6f * fRec10[1];
			fVec2[IOTA0 & 4095] = fTemp13;
			fRec10[0] = fVec2[(IOTA0 - iConst14) & 4095];
			float fRec11 = 0.6f * fTemp13;
			float fTemp14 = std::exp(-(fConst17 / fTemp3));
			float fTemp15 = ReverbDsp_faustpower2_f(fTemp14);
			float fTemp16 = 1.0f - fTemp15;
			float fTemp17 = 1.0f - fTemp15 * fTemp7;
			float fTemp18 = std::sqrt(std::max<float>(0.0f, ReverbDsp_faustpower2_f(fTemp17) / ReverbDsp_faustpower2_f(fTemp16) + -1.0f));
			float fTemp19 = fTemp17 / fTemp16;
			fRec19[0] = -(fConst10 * (fConst9 * fRec19[1] - (fRec2[1] + fRec2[2])));
			fRec18[0] = fTemp14 * (fTemp18 + (1.0f - fTemp19)) * (fRec2[1] + fRec19[0] * (std::exp(-(fConst18 / fTemp3)) / fTemp14 + -1.0f)) + (fTemp19 - fTemp18) * fRec18[1];
			fVec3[IOTA0 & 16383] = 0.35355338f * fRec18[0] + 1e-20f;
			float fTemp20 = fVec3[(IOTA0 - iConst20) & 16383] + fTemp12 - 0.6f * fRec16[1];
			fVec4[IOTA0 & 2047] = fTemp20;
			fRec16[0] = fVec4[(IOTA0 - iConst21) & 2047];
			float fRec17 = 0.6f * fTemp20;
			float fTemp21 = fRec17 + fRec11;
			float fTemp22 = std::exp(-(fConst24 / fTemp3));
			float fTemp23 = ReverbDsp_faustpower2_f(fTemp22);
			float fTemp24 = 1.0f - fTemp23;
			float fTemp25 = 1.0f - fTemp7 * fTemp23;
			float fTemp26 = std::sqrt(std::max<float>(0.0f, ReverbDsp_faustpower2_f(fTemp25) / ReverbDsp_faustpower2_f(fTemp24) + -1.0f));
			float fTemp27 = fTemp25 / fTemp24;
			fRec23[0] = -(fConst10 * (fConst9 * fRec23[1] - (fRec4[1] + fRec4[2])));
			fRec22[0] = fTemp22 * (fTemp26 + (1.0f - fTemp27)) * (fRec4[1] + fRec23[0] * (std::exp(-(fConst25 / fTemp3)) / fTemp22 + -1.0f)) + (fTemp27 - fTemp26) * fRec22[1];
			fVec5[IOTA0 & 16383] = 0.35355338f * fRec22[0] + 1e-20f;
			float fTemp28 = fVec5[(IOTA0 - iConst27) & 16383] - (fTemp12 + 0.6f * fRec20[1]);
			fVec6[IOTA0 & 4095] = fTemp28;
			fRec20[0] = fVec6[(IOTA0 - iConst28) & 4095];
			float fRec21 = 0.6f * fTemp28;
			float fTemp29 = std::exp(-(fConst31 / fTemp3));
			float fTemp30 = ReverbDsp_faustpower2_f(fTemp29);
			float fTemp31 = 1.0f - fTemp30;
			float fTemp32 = 1.0f - fTemp7 * fTemp30;
			float fTemp33 = std::sqrt(std::max<float>(0.0f, ReverbDsp_faustpower2_f(fTemp32) / ReverbDsp_faustpower2_f(fTemp31) + -1.0f));
			float fTemp34 = fTemp32 / fTemp31;
			fRec27[0] = -(fConst10 * (fConst9 * fRec27[1] - (fRec8[1] + fRec8[2])));
			fRec26[0] = fTemp29 * (fTemp33 + (1.0f - fTemp34)) * (fRec8[1] + fRec27[0] * (std::exp(-(fConst32 / fTemp3)) / fTemp29 + -1.0f)) + (fTemp34 - fTemp33) * fRec26[1];
			fVec7[IOTA0 & 16383] = 0.35355338f * fRec26[0] + 1e-20f;
			float fTemp35 = fVec7[(IOTA0 - iConst34) & 16383] - (fTemp12 + 0.6f * fRec24[1]);
			fVec8[IOTA0 & 2047] = fTemp35;
			fRec24[0] = fVec8[(IOTA0 - iConst35) & 2047];
			float fRec25 = 0.6f * fTemp35;
			float fTemp36 = fRec25 + fRec21 + fTemp21;
			float fTemp37 = std::exp(-(fConst38 / fTemp3));
			float fTemp38 = ReverbDsp_faustpower2_f(fTemp37);
			float fTemp39 = 1.0f - fTemp38;
			float fTemp40 = 1.0f - fTemp7 * fTemp38;
			float fTemp41 = std::sqrt(std::max<float>(0.0f, ReverbDsp_faustpower2_f(fTemp40) / ReverbDsp_faustpower2_f(fTemp39) + -1.0f));
			float fTemp42 = fTemp40 / fTemp39;
			fRec31[0] = -(fConst10 * (fConst9 * fRec31[1] - (fRec3[1] + fRec3[2])));
			fRec30[0] = fTemp37 * (fTemp41 + (1.0f - fTemp42)) * (fRec3[1] + fRec31[0] * (std::exp(-(fConst39 / fTemp3)) / fTemp37 + -1.0f)) + (fTemp42 - fTemp41) * fRec30[1];
			fVec9[IOTA0 & 32767] = 0.35355338f * fRec30[0] + 1e-20f;
			fVec10[IOTA0 & 32767] = float(input1[i0]);
			float fTemp43 = 0.3f * fVec10[(IOTA0 - iTemp11) & 32767];
			float fTemp44 = fTemp43 + 0.6f * fRec28[1] + fVec9[(IOTA0 - iConst41) & 32767];
			fVec11[IOTA0 & 4095] = fTemp44;
			fRec28[0] = fVec11[(IOTA0 - iConst42) & 4095];
			float fRec29 = -(0.6f * fTemp44);
			float fTemp45 = std::exp(-(fConst45 / fTemp3));
			float fTemp46 = ReverbDsp_faustpower2_f(fTemp45);
			float fTemp47 = 1.0f - fTemp46;
			float fTemp48 = 1.0f - fTemp7 * fTemp46;
			float fTemp49 = std::sqrt(std::max<float>(0.0f, ReverbDsp_faustpower2_f(fTemp48) / ReverbDsp_faustpower2_f(fTemp47) + -1.0f));
			float fTemp50 = fTemp48 / fTemp47;
			fRec35[0] = -(fConst10 * (fConst9 * fRec35[1] - (fRec7[1] + fRec7[2])));
			fRec34[0] = fTemp45 * (fTemp49 + (1.0f - fTemp50)) * (fRec7[1] + fRec35[0] * (std::exp(-(fConst46 / fTemp3)) / fTemp45 + -1.0f)) + (fTemp50 - fTemp49) * fRec34[1];
			fVec12[IOTA0 & 16383] = 0.35355338f * fRec34[0] + 1e-20f;
			float fTemp51 = fVec12[(IOTA0 - iConst48) & 16383] + fTemp43 + 0.6f * fRec32[1];
			fVec13[IOTA0 & 4095] = fTemp51;
			fRec32[0] = fVec13[(IOTA0 - iConst49) & 4095];
			float fRec33 = -(0.6f * fTemp51);
			float fTemp52 = std::exp(-(fConst52 / fTemp3));
			float fTemp53 = ReverbDsp_faustpower2_f(fTemp52);
			float fTemp54 = 1.0f - fTemp53;
			float fTemp55 = 1.0f - fTemp7 * fTemp53;
			float fTemp56 = std::sqrt(std::max<float>(0.0f, ReverbDsp_faustpower2_f(fTemp55) / ReverbDsp_faustpower2_f(fTemp54) + -1.0f));
			float fTemp57 = fTemp55 / fTemp54;
			fRec39[0] = -(fConst10 * (fConst9 * fRec39[1] - (fRec5[1] + fRec5[2])));
			fRec38[0] = fTemp52 * (fTemp56 + (1.0f - fTemp57)) * (fRec5[1] + fRec39[0] * (std::exp(-(fConst53 / fTemp3)) / fTemp52 + -1.0f)) + (fTemp57 - fTemp56) * fRec38[1];
			fVec14[IOTA0 & 32767] = 0.35355338f * fRec38[0] + 1e-20f;
			float fTemp58 = 0.6f * fRec36[1] + fVec14[(IOTA0 - iConst55) & 32767];
			fVec15[IOTA0 & 4095] = fTemp58 - fTemp43;
			fRec36[0] = fVec15[(IOTA0 - iConst56) & 4095];
			float fRec37 = 0.6f * (fTemp43 - fTemp58);
			float fTemp59 = std::exp(-(fConst59 / fTemp3));
			float fTemp60 = ReverbDsp_faustpower2_f(fTemp59);
			float fTemp61 = 1.0f - fTemp60;
			float fTemp62 = 1.0f - fTemp7 * fTemp60;
			float fTemp63 = std::sqrt(std::max<float>(0.0f, ReverbDsp_faustpower2_f(fTemp62) / ReverbDsp_faustpower2_f(fTemp61) + -1.0f));
			float fTemp64 = fTemp62 / fTemp61;
			fRec43[0] = -(fConst10 * (fConst9 * fRec43[1] - (fRec9[1] + fRec9[2])));
			fRec42[0] = fTemp59 * (fTemp63 + (1.0f - fTemp64)) * (fRec9[1] + fRec43[0] * (std::exp(-(fConst60 / fTemp3)) / fTemp59 + -1.0f)) + (fTemp64 - fTemp63) * fRec42[1];
			fVec16[IOTA0 & 32767] = 0.35355338f * fRec42[0] + 1e-20f;
			float fTemp65 = 0.6f * fRec40[1] + fVec16[(IOTA0 - iConst62) & 32767];
			fVec17[IOTA0 & 2047] = fTemp65 - fTemp43;
			fRec40[0] = fVec17[(IOTA0 - iConst63) & 2047];
			float fRec41 = 0.6f * (fTemp43 - fTemp65);
			fRec2[0] = fRec40[1] + fRec36[1] + fRec32[1] + fRec28[1] + fRec24[1] + fRec20[1] + fRec10[1] + fRec16[1] + fRec41 + fRec37 + fRec33 + fRec29 + fTemp36;
			fRec3[0] = fRec24[1] + fRec20[1] + fRec10[1] + fRec16[1] + fTemp36 - (fRec40[1] + fRec36[1] + fRec32[1] + fRec28[1] + fRec41 + fRec37 + fRec29 + fRec33);
			float fTemp66 = fRec21 + fRec25;
			fRec4[0] = fRec32[1] + fRec28[1] + fRec10[1] + fRec16[1] + fRec33 + fRec29 + fTemp21 - (fRec40[1] + fRec36[1] + fRec24[1] + fRec20[1] + fRec41 + fRec37 + fTemp66);
			fRec5[0] = fRec40[1] + fRec36[1] + fRec10[1] + fRec16[1] + fRec41 + fRec37 + fTemp21 - (fRec32[1] + fRec28[1] + fRec24[1] + fRec20[1] + fRec33 + fRec29 + fTemp66);
			float fTemp67 = fRec11 + fRec25;
			float fTemp68 = fRec17 + fRec21;
			fRec6[0] = fRec36[1] + fRec28[1] + fRec20[1] + fRec16[1] + fRec37 + fRec29 + fTemp68 - (fRec40[1] + fRec32[1] + fRec24[1] + fRec10[1] + fRec41 + fRec33 + fTemp67);
			fRec7[0] = fRec40[1] + fRec32[1] + fRec20[1] + fRec16[1] + fRec41 + fRec33 + fTemp68 - (fRec36[1] + fRec28[1] + fRec24[1] + fRec10[1] + fRec37 + fRec29 + fTemp67);
			float fTemp69 = fRec11 + fRec21;
			float fTemp70 = fRec17 + fRec25;
			fRec8[0] = fRec40[1] + fRec28[1] + fRec24[1] + fRec16[1] + fRec41 + fRec29 + fTemp70 - (fRec36[1] + fRec32[1] + fRec20[1] + fRec10[1] + fRec37 + fRec33 + fTemp69);
			fRec9[0] = fRec36[1] + fRec32[1] + fRec24[1] + fRec16[1] + fRec37 + fRec33 + fTemp70 - (fRec40[1] + fRec28[1] + fRec20[1] + fRec10[1] + fRec41 + fRec29 + fTemp69);
			float fTemp71 = fRec3[0] + fRec4[0];
			fVec18[0] = fTemp71;
			fRec0[0] = (0.37f * (fTemp71 + fVec18[1]) - fRec0[1] * fTemp2) / fTemp1;
			output0[i0] = FAUSTFLOAT(fRec0[0]);
			float fTemp72 = fRec3[0] - fRec4[0];
			fVec19[0] = fTemp72;
			fRec44[0] = (0.37f * (fTemp72 + fVec19[1]) - fTemp2 * fRec44[1]) / fTemp1;
			output1[i0] = FAUSTFLOAT(fRec44[0]);
			fRec1[1] = fRec1[0];
			fRec13[1] = fRec13[0];
			fRec14[1] = fRec14[0];
			fRec12[1] = fRec12[0];
			IOTA0 = IOTA0 + 1;
			fRec15[1] = fRec15[0];
			fRec10[1] = fRec10[0];
			fRec19[1] = fRec19[0];
			fRec18[1] = fRec18[0];
			fRec16[1] = fRec16[0];
			fRec23[1] = fRec23[0];
			fRec22[1] = fRec22[0];
			fRec20[1] = fRec20[0];
			fRec27[1] = fRec27[0];
			fRec26[1] = fRec26[0];
			fRec24[1] = fRec24[0];
			fRec31[1] = fRec31[0];
			fRec30[1] = fRec30[0];
			fRec28[1] = fRec28[0];
			fRec35[1] = fRec35[0];
			fRec34[1] = fRec34[0];
			fRec32[1] = fRec32[0];
			fRec39[1] = fRec39[0];
			fRec38[1] = fRec38[0];
			fRec36[1] = fRec36[0];
			fRec43[1] = fRec43[0];
			fRec42[1] = fRec42[0];
			fRec40[1] = fRec40[0];
			fRec2[2] = fRec2[1];
			fRec2[1] = fRec2[0];
			fRec3[2] = fRec3[1];
			fRec3[1] = fRec3[0];
			fRec4[2] = fRec4[1];
			fRec4[1] = fRec4[0];
			fRec5[2] = fRec5[1];
			fRec5[1] = fRec5[0];
			fRec6[2] = fRec6[1];
			fRec6[1] = fRec6[0];
			fRec7[2] = fRec7[1];
			fRec7[1] = fRec7[0];
			fRec8[2] = fRec8[1];
			fRec8[1] = fRec8[0];
			fRec9[2] = fRec9[1];
			fRec9[1] = fRec9[0];
			fVec18[1] = fVec18[0];
			fRec0[1] = fRec0[0];
			fVec19[1] = fVec19[0];
			fRec44[1] = fRec44[0];
		}
	}

};
} // namespace fxfaust

#endif
