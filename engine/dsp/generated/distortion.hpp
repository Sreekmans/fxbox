/* ------------------------------------------------------------
license: "GPL-3.0-or-later"
name: "fxbox_distortion"
Code generated with Faust 2.70.3 (https://faust.grame.fr)
Compilation options: -a arch_minimal.cpp -lang cpp -ct 1 -cn DistortionDsp -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0
------------------------------------------------------------ */

#ifndef  __DistortionDsp_H__
#define  __DistortionDsp_H__

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

/* link with : "" */
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <math.h>

#ifndef FAUSTCLASS 
#define FAUSTCLASS DistortionDsp
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

static float DistortionDsp_faustpower2_f(float value) {
	return value * value;
}

class DistortionDsp : public dsp {
	
 private:
	
	int fSampleRate;
	float fConst1;
	float fConst2;
	FAUSTFLOAT fHslider0;
	float fRec0[2];
	float fConst3;
	FAUSTFLOAT fHslider1;
	float fRec1[2];
	FAUSTFLOAT fHslider2;
	float fRec4[2];
	FAUSTFLOAT fHslider3;
	float fRec5[2];
	float fConst4;
	float fVec0[2];
	float fConst5;
	float fConst6;
	float fConst7;
	float fRec8[2];
	float fConst8;
	float fConst9;
	float fRec7[2];
	float fConst10;
	float fRec6[2];
	float fConst11;
	FAUSTFLOAT fHslider4;
	float fConst12;
	float fRec9[2];
	FAUSTFLOAT fHslider5;
	float fRec10[2];
	int iRec12[2];
	float fRec11[2];
	float fRec13[2];
	float fRec14[2];
	float fRec15[2];
	float fRec16[2];
	float fVec1[2];
	float fRec3[2];
	float fRec2[3];
	float fVec2[2];
	float fRec21[2];
	float fRec20[2];
	float fRec19[2];
	float fRec22[2];
	float fVec3[2];
	float fRec18[2];
	float fRec17[3];
	
 public:
	DistortionDsp() {}

	void metadata(Meta* m) { 
		m->declare("aanl.lib/name", "Faust Antialiased Nonlinearities");
		m->declare("aanl.lib/version", "1.3.0");
		m->declare("analyzers.lib/amp_follower_ar:author", "Jonatan Liljedahl, revised by Romain Michon");
		m->declare("analyzers.lib/name", "Faust Analyzer Library");
		m->declare("analyzers.lib/version", "1.2.0");
		m->declare("basics.lib/downSample:author", "Romain Michon");
		m->declare("basics.lib/name", "Faust Basic Element Library");
		m->declare("basics.lib/sAndH:author", "Romain Michon");
		m->declare("basics.lib/tabulateNd", "Copyright (C) 2023 Bart Brouns <bart@magnetophon.nl>");
		m->declare("basics.lib/version", "1.12.0");
		m->declare("compile_options", "-a arch_minimal.cpp -lang cpp -ct 1 -cn DistortionDsp -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0");
		m->declare("filename", "distortion.dsp");
		m->declare("filters.lib/dcblocker:author", "Julius O. Smith III");
		m->declare("filters.lib/dcblocker:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/dcblocker:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/fir:author", "Julius O. Smith III");
		m->declare("filters.lib/fir:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/fir:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/highpass:author", "Julius O. Smith III");
		m->declare("filters.lib/highpass:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/iir:author", "Julius O. Smith III");
		m->declare("filters.lib/iir:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/iir:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/lowpass0_highpass1", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/lowpass0_highpass1:author", "Julius O. Smith III");
		m->declare("filters.lib/lowpass:author", "Julius O. Smith III");
		m->declare("filters.lib/lowpass:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/lowpass:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/name", "Faust Filters Library");
		m->declare("filters.lib/pole:author", "Julius O. Smith III");
		m->declare("filters.lib/pole:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/pole:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/tf1:author", "Julius O. Smith III");
		m->declare("filters.lib/tf1:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/tf1:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/tf1s:author", "Julius O. Smith III");
		m->declare("filters.lib/tf1s:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/tf1s:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/tf2:author", "Julius O. Smith III");
		m->declare("filters.lib/tf2:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/tf2:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/tf2s:author", "Julius O. Smith III");
		m->declare("filters.lib/tf2s:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/tf2s:license", "MIT-style STK-4.3 license");
		m->declare("filters.lib/version", "1.3.0");
		m->declare("filters.lib/zero:author", "Julius O. Smith III");
		m->declare("filters.lib/zero:copyright", "Copyright (C) 2003-2019 by Julius O. Smith III <jos@ccrma.stanford.edu>");
		m->declare("filters.lib/zero:license", "MIT-style STK-4.3 license");
		m->declare("license", "GPL-3.0-or-later");
		m->declare("maths.lib/author", "GRAME");
		m->declare("maths.lib/copyright", "GRAME");
		m->declare("maths.lib/license", "LGPL with exception");
		m->declare("maths.lib/name", "Faust Math Library");
		m->declare("maths.lib/version", "2.7.0");
		m->declare("misceffects.lib/name", "Misc Effects Library");
		m->declare("misceffects.lib/version", "2.4.0");
		m->declare("misceffects.lib/wavefold:author", "David Braun");
		m->declare("name", "fxbox_distortion");
		m->declare("platform.lib/name", "Generic Platform Library");
		m->declare("platform.lib/version", "1.3.0");
		m->declare("signals.lib/name", "Faust Signal Routing Library");
		m->declare("signals.lib/onePoleSwitching:author", "Jonatan Liljedahl, revised by Dario Sanfilippo");
		m->declare("signals.lib/onePoleSwitching:licence", "STK-4.3");
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
		fConst3 = 2199.1147f / fConst0;
		fConst4 = std::exp(-(1e+02f / fConst0));
		fConst5 = 1.0f / std::tan(376.99112f / fConst0);
		fConst6 = 1.0f - fConst5;
		fConst7 = 1.0f / (fConst5 + 1.0f);
		fConst8 = std::exp(-(12.5f / fConst0));
		fConst9 = std::exp(-(1e+03f / fConst0));
		fConst10 = 1.0f - fConst4;
		fConst11 = std::exp(-(5e+01f / fConst0));
		fConst12 = 1.0f - fConst11;
	}
	
	virtual void instanceResetUserInterface() {
		fHslider0 = FAUSTFLOAT(0.5f);
		fHslider1 = FAUSTFLOAT(-6.0f);
		fHslider2 = FAUSTFLOAT(0.5f);
		fHslider3 = FAUSTFLOAT(0.0f);
		fHslider4 = FAUSTFLOAT(0.0f);
		fHslider5 = FAUSTFLOAT(0.5f);
	}
	
	virtual void instanceClear() {
		for (int l0 = 0; l0 < 2; l0 = l0 + 1) {
			fRec0[l0] = 0.0f;
		}
		for (int l1 = 0; l1 < 2; l1 = l1 + 1) {
			fRec1[l1] = 0.0f;
		}
		for (int l2 = 0; l2 < 2; l2 = l2 + 1) {
			fRec4[l2] = 0.0f;
		}
		for (int l3 = 0; l3 < 2; l3 = l3 + 1) {
			fRec5[l3] = 0.0f;
		}
		for (int l4 = 0; l4 < 2; l4 = l4 + 1) {
			fVec0[l4] = 0.0f;
		}
		for (int l5 = 0; l5 < 2; l5 = l5 + 1) {
			fRec8[l5] = 0.0f;
		}
		for (int l6 = 0; l6 < 2; l6 = l6 + 1) {
			fRec7[l6] = 0.0f;
		}
		for (int l7 = 0; l7 < 2; l7 = l7 + 1) {
			fRec6[l7] = 0.0f;
		}
		for (int l8 = 0; l8 < 2; l8 = l8 + 1) {
			fRec9[l8] = 0.0f;
		}
		for (int l9 = 0; l9 < 2; l9 = l9 + 1) {
			fRec10[l9] = 0.0f;
		}
		for (int l10 = 0; l10 < 2; l10 = l10 + 1) {
			iRec12[l10] = 0;
		}
		for (int l11 = 0; l11 < 2; l11 = l11 + 1) {
			fRec11[l11] = 0.0f;
		}
		for (int l12 = 0; l12 < 2; l12 = l12 + 1) {
			fRec13[l12] = 0.0f;
		}
		for (int l13 = 0; l13 < 2; l13 = l13 + 1) {
			fRec14[l13] = 0.0f;
		}
		for (int l14 = 0; l14 < 2; l14 = l14 + 1) {
			fRec15[l14] = 0.0f;
		}
		for (int l15 = 0; l15 < 2; l15 = l15 + 1) {
			fRec16[l15] = 0.0f;
		}
		for (int l16 = 0; l16 < 2; l16 = l16 + 1) {
			fVec1[l16] = 0.0f;
		}
		for (int l17 = 0; l17 < 2; l17 = l17 + 1) {
			fRec3[l17] = 0.0f;
		}
		for (int l18 = 0; l18 < 3; l18 = l18 + 1) {
			fRec2[l18] = 0.0f;
		}
		for (int l19 = 0; l19 < 2; l19 = l19 + 1) {
			fVec2[l19] = 0.0f;
		}
		for (int l20 = 0; l20 < 2; l20 = l20 + 1) {
			fRec21[l20] = 0.0f;
		}
		for (int l21 = 0; l21 < 2; l21 = l21 + 1) {
			fRec20[l21] = 0.0f;
		}
		for (int l22 = 0; l22 < 2; l22 = l22 + 1) {
			fRec19[l22] = 0.0f;
		}
		for (int l23 = 0; l23 < 2; l23 = l23 + 1) {
			fRec22[l23] = 0.0f;
		}
		for (int l24 = 0; l24 < 2; l24 = l24 + 1) {
			fVec3[l24] = 0.0f;
		}
		for (int l25 = 0; l25 < 2; l25 = l25 + 1) {
			fRec18[l25] = 0.0f;
		}
		for (int l26 = 0; l26 < 3; l26 = l26 + 1) {
			fRec17[l26] = 0.0f;
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
	
	virtual DistortionDsp* clone() {
		return new DistortionDsp();
	}
	
	virtual int getSampleRate() {
		return fSampleRate;
	}
	
	virtual void buildUserInterface(UI* ui_interface) {
		ui_interface->openVerticalBox("fxbox_distortion");
		ui_interface->addHorizontalSlider("crush", &fHslider5, FAUSTFLOAT(0.5f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(0.0001f));
		ui_interface->addHorizontalSlider("drive", &fHslider2, FAUSTFLOAT(0.5f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(0.0001f));
		ui_interface->addHorizontalSlider("gate", &fHslider3, FAUSTFLOAT(0.0f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(0.0001f));
		ui_interface->addHorizontalSlider("level", &fHslider1, FAUSTFLOAT(-6.0f), FAUSTFLOAT(-24.0f), FAUSTFLOAT(6.0f), FAUSTFLOAT(0.01f));
		ui_interface->addHorizontalSlider("mode", &fHslider4, FAUSTFLOAT(0.0f), FAUSTFLOAT(0.0f), FAUSTFLOAT(4.0f), FAUSTFLOAT(1.0f));
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
		float fSlow3 = fConst1 * float(fHslider3);
		float fSlow4 = float(fHslider4);
		float fSlow5 = fConst12 * float(fSlow4 == 4.0f);
		float fSlow6 = fConst1 * float(fHslider5);
		float fSlow7 = fConst12 * float(fSlow4 == 3.0f);
		float fSlow8 = fConst12 * float(fSlow4 == 2.0f);
		float fSlow9 = fConst12 * float(fSlow4 == 1.0f);
		float fSlow10 = fConst12 * float(fSlow4 == 0.0f);
		for (int i0 = 0; i0 < count; i0 = i0 + 1) {
			fRec0[0] = fSlow0 + fConst2 * fRec0[1];
			float fTemp0 = std::tan(fConst3 * std::pow(14.0f, fRec0[0]));
			float fTemp1 = 1.0f / fTemp0;
			float fTemp2 = (fTemp1 + 1.4142135f) / fTemp0 + 1.0f;
			fRec1[0] = fSlow1 + fConst2 * fRec1[1];
			float fTemp3 = std::pow(1e+01f, 0.05f * fRec1[0]);
			float fTemp4 = 1.0f - 1.0f / DistortionDsp_faustpower2_f(fTemp0);
			float fTemp5 = (fTemp1 + -1.4142135f) / fTemp0 + 1.0f;
			fRec4[0] = fSlow2 + fConst2 * fRec4[1];
			float fTemp6 = 8.0f * fRec4[0] + 1.0f;
			fRec5[0] = fSlow3 + fConst2 * fRec5[1];
			float fTemp7 = float(fRec5[0] == 0.0f);
			float fTemp8 = std::pow(1e+01f, 0.05f * (5e+01f * fRec5[0] + -8e+01f));
			float fTemp9 = float(input0[i0]);
			fVec0[0] = fTemp9;
			fRec8[0] = -(fConst7 * (fConst6 * fRec8[1] - fConst5 * (fTemp9 - fVec0[1])));
			float fTemp10 = std::fabs(fRec8[0]);
			float fTemp11 = ((fTemp10 > fRec7[1]) ? fConst9 : fConst8);
			fRec7[0] = fTemp10 * (1.0f - fTemp11) + fRec7[1] * fTemp11;
			fRec6[0] = fConst10 * float(fRec7[0] > fTemp8) + fConst4 * fRec6[1];
			float fTemp12 = fRec8[0] * std::max<float>(fTemp7, fRec6[0]);
			float fTemp13 = fTemp12 * fTemp6;
			int iTemp14 = fTemp13 < 0.0f;
			float fTemp15 = std::max<float>(0.0f, std::min<float>(1.0f, 0.7f * fRec4[0] + 0.3f));
			float fTemp16 = 0.8f * fTemp15;
			float fTemp17 = 1.0f - fTemp16;
			float fTemp18 = ((iTemp14) ? -fTemp13 : fTemp13);
			float fTemp19 = 1.25f * ((fTemp18 + fTemp16 + -1.0f) / fTemp15);
			float fTemp20 = ((fTemp18 > fTemp17) ? fTemp15 * (0.4f * std::fabs(fTemp19 + (-0.5f - std::floor(fTemp19))) + -1.0f) + 1.0f : fTemp18) / fTemp17;
			fRec9[0] = fSlow5 + fConst11 * fRec9[1];
			float fTemp21 = 0.8f * fRec9[0] * ((iTemp14) ? -fTemp20 : fTemp20);
			fRec10[0] = fSlow6 + fConst2 * fRec10[1];
			float fTemp22 = std::pow(2.0f, 15.0f - 12.0f * fRec10[0]);
			iRec12[0] = iRec12[1] + 1;
			int iTemp23 = ((iRec12[0] + -1) % int(31.0f * fRec10[0] + 1.0f)) == 0;
			float fTemp24 = 4.0f * fRec4[0] + 1.0f;
			fRec11[0] = ((iTemp23) ? fTemp12 * fTemp24 : fRec11[1]);
			fRec13[0] = fSlow7 + fConst11 * fRec13[1];
			float fTemp25 = fRec13[0] * std::min<float>(1.0f, std::max<float>(-1.0f, std::floor(fRec11[0] * fTemp22) / fTemp22));
			float fTemp26 = std::pow(1e+01f, 2.0f * fRec4[0]);
			float fTemp27 = fTemp12 * fTemp26;
			float fTemp28 = 0.35f * fRec4[0];
			float fTemp29 = fTemp28 + 2.0f * fTemp27;
			fRec14[0] = fSlow8 + fConst11 * fRec14[1];
			float fTemp30 = fRec14[0] * float(((fTemp29 >= 0.0f) ? 1 : -1)) * (1.0f - std::exp(-(std::fabs(fTemp29))));
			float fTemp31 = 0.2f * fRec4[0];
			float fTemp32 = std::min<float>(0.7f, std::max<float>(-0.7f, fTemp31));
			float fTemp33 = fTemp31 + fTemp27;
			fRec15[0] = fSlow9 + fConst11 * fRec15[1];
			float fTemp34 = fRec15[0] * (std::min<float>(0.7f, std::max<float>(-0.7f, fTemp33)) - fTemp32);
			float fTemp35 = tanhf(fTemp31);
			fRec16[0] = fSlow10 + fConst11 * fRec16[1];
			float fTemp36 = fRec16[0] * (tanhf(fTemp33) - fTemp35);
			fVec1[0] = fTemp36 + fTemp34 + fTemp30 + fTemp25 + fTemp21;
			fRec3[0] = fTemp21 + fTemp25 + fTemp30 + fTemp34 + 0.995f * fRec3[1] + fTemp36 - fVec1[1];
			fRec2[0] = fRec3[0] - (fRec2[2] * fTemp5 + 2.0f * fRec2[1] * fTemp4) / fTemp2;
			output0[i0] = FAUSTFLOAT((fRec2[2] + fRec2[0] + 2.0f * fRec2[1]) * fTemp3 / fTemp2);
			float fTemp37 = float(input1[i0]);
			fVec2[0] = fTemp37;
			fRec21[0] = -(fConst7 * (fConst6 * fRec21[1] - fConst5 * (fTemp37 - fVec2[1])));
			float fTemp38 = std::fabs(fRec21[0]);
			float fTemp39 = ((fTemp38 > fRec20[1]) ? fConst9 : fConst8);
			fRec20[0] = fTemp38 * (1.0f - fTemp39) + fRec20[1] * fTemp39;
			fRec19[0] = fConst10 * float(fRec20[0] > fTemp8) + fConst4 * fRec19[1];
			float fTemp40 = fRec21[0] * std::max<float>(fTemp7, fRec19[0]);
			float fTemp41 = fTemp40 * fTemp6;
			int iTemp42 = fTemp41 < 0.0f;
			float fTemp43 = ((iTemp42) ? -fTemp41 : fTemp41);
			float fTemp44 = 1.25f * ((fTemp16 + fTemp43 + -1.0f) / fTemp15);
			float fTemp45 = ((fTemp43 > fTemp17) ? fTemp15 * (0.4f * std::fabs(fTemp44 + (-0.5f - std::floor(fTemp44))) + -1.0f) + 1.0f : fTemp43) / fTemp17;
			float fTemp46 = 0.8f * fRec9[0] * ((iTemp42) ? -fTemp45 : fTemp45);
			fRec22[0] = ((iTemp23) ? fTemp40 * fTemp24 : fRec22[1]);
			float fTemp47 = fRec13[0] * std::min<float>(1.0f, std::max<float>(-1.0f, std::floor(fRec22[0] * fTemp22) / fTemp22));
			float fTemp48 = fTemp40 * fTemp26;
			float fTemp49 = fTemp28 + 2.0f * fTemp48;
			float fTemp50 = fRec14[0] * float(((fTemp49 >= 0.0f) ? 1 : -1)) * (1.0f - std::exp(-(std::fabs(fTemp49))));
			float fTemp51 = fTemp31 + fTemp48;
			float fTemp52 = fRec15[0] * (std::min<float>(0.7f, std::max<float>(-0.7f, fTemp51)) - fTemp32);
			float fTemp53 = fRec16[0] * (tanhf(fTemp51) - fTemp35);
			fVec3[0] = fTemp53 + fTemp52 + fTemp50 + fTemp47 + fTemp46;
			fRec18[0] = fTemp46 + fTemp47 + fTemp50 + fTemp52 + 0.995f * fRec18[1] + fTemp53 - fVec3[1];
			fRec17[0] = fRec18[0] - (fTemp5 * fRec17[2] + 2.0f * fTemp4 * fRec17[1]) / fTemp2;
			output1[i0] = FAUSTFLOAT(fTemp3 * (fRec17[2] + fRec17[0] + 2.0f * fRec17[1]) / fTemp2);
			fRec0[1] = fRec0[0];
			fRec1[1] = fRec1[0];
			fRec4[1] = fRec4[0];
			fRec5[1] = fRec5[0];
			fVec0[1] = fVec0[0];
			fRec8[1] = fRec8[0];
			fRec7[1] = fRec7[0];
			fRec6[1] = fRec6[0];
			fRec9[1] = fRec9[0];
			fRec10[1] = fRec10[0];
			iRec12[1] = iRec12[0];
			fRec11[1] = fRec11[0];
			fRec13[1] = fRec13[0];
			fRec14[1] = fRec14[0];
			fRec15[1] = fRec15[0];
			fRec16[1] = fRec16[0];
			fVec1[1] = fVec1[0];
			fRec3[1] = fRec3[0];
			fRec2[2] = fRec2[1];
			fRec2[1] = fRec2[0];
			fVec2[1] = fVec2[0];
			fRec21[1] = fRec21[0];
			fRec20[1] = fRec20[0];
			fRec19[1] = fRec19[0];
			fRec22[1] = fRec22[0];
			fVec3[1] = fVec3[0];
			fRec18[1] = fRec18[0];
			fRec17[2] = fRec17[1];
			fRec17[1] = fRec17[0];
		}
	}

};
} // namespace fxfaust

#endif
