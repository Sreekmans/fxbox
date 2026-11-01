/* ------------------------------------------------------------
license: "GPL-3.0-or-later"
name: "fxbox_compressor"
Code generated with Faust 2.70.3 (https://faust.grame.fr)
Compilation options: -a arch_minimal.cpp -lang cpp -ct 1 -cn CompressorDsp -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0
------------------------------------------------------------ */

#ifndef  __CompressorDsp_H__
#define  __CompressorDsp_H__

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
#define FAUSTCLASS CompressorDsp
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

static float CompressorDsp_faustpower2_f(float value) {
	return value * value;
}

class CompressorDsp : public dsp {
	
 private:
	
	int fSampleRate;
	float fConst1;
	float fConst2;
	FAUSTFLOAT fHslider0;
	float fRec1[2];
	FAUSTFLOAT fHslider1;
	float fRec2[2];
	FAUSTFLOAT fHslider2;
	float fRec3[2];
	FAUSTFLOAT fHslider3;
	float fRec5[2];
	float fConst3;
	FAUSTFLOAT fHslider4;
	float fRec6[2];
	float fRec4[2];
	FAUSTFLOAT fHslider5;
	float fRec7[2];
	float fRec8[2];
	FAUSTFLOAT fHbargraph0;
	float fConst4;
	float fConst5;
	float fRec0[2];
	float fRec9[2];
	float fConst6;
	FAUSTFLOAT fHslider6;
	float fConst7;
	float fRec10[2];
	
 public:
	CompressorDsp() {}

	void metadata(Meta* m) { 
		m->declare("basics.lib/bypass1:author", "Julius Smith");
		m->declare("basics.lib/name", "Faust Basic Element Library");
		m->declare("basics.lib/parallelMin:author", "Bart Brouns");
		m->declare("basics.lib/parallelMin:copyright", "Copyright (c) 2020 Bart Brouns <bart@magnetophon.nl>");
		m->declare("basics.lib/parallelMin:licence", "GPL-3.0");
		m->declare("basics.lib/parallelOp:author", "Bart Brouns");
		m->declare("basics.lib/parallelOp:copyright", "Copyright (c) 2020 Bart Brouns <bart@magnetophon.nl>");
		m->declare("basics.lib/parallelOp:licence", "GPL-3.0");
		m->declare("basics.lib/tabulateNd", "Copyright (C) 2023 Bart Brouns <bart@magnetophon.nl>");
		m->declare("basics.lib/version", "1.12.0");
		m->declare("compile_options", "-a arch_minimal.cpp -lang cpp -ct 1 -cn CompressorDsp -es 1 -mcd 16 -mdd 1024 -mdy 33 -single -ftz 0");
		m->declare("compressors.lib/FFcompressor_N_chan:author", "Bart Brouns");
		m->declare("compressors.lib/FFcompressor_N_chan:license", "GPLv3");
		m->declare("compressors.lib/name", "Faust Compressor Effect Library");
		m->declare("compressors.lib/peak_compression_gain_N_chan_db:author", "Bart Brouns");
		m->declare("compressors.lib/peak_compression_gain_N_chan_db:license", "GPLv3");
		m->declare("compressors.lib/peak_compression_gain_mono_db:author", "Bart Brouns");
		m->declare("compressors.lib/peak_compression_gain_mono_db:license", "GPLv3");
		m->declare("compressors.lib/version", "1.6.0");
		m->declare("filename", "compressor.dsp");
		m->declare("interpolators.lib/interpolate_linear:author", "Stéphane Letz");
		m->declare("interpolators.lib/interpolate_linear:licence", "MIT");
		m->declare("interpolators.lib/name", "Faust Interpolator Library");
		m->declare("interpolators.lib/version", "1.3.0");
		m->declare("license", "GPL-3.0-or-later");
		m->declare("maths.lib/author", "GRAME");
		m->declare("maths.lib/copyright", "GRAME");
		m->declare("maths.lib/license", "LGPL with exception");
		m->declare("maths.lib/name", "Faust Math Library");
		m->declare("maths.lib/version", "2.7.0");
		m->declare("name", "fxbox_compressor");
		m->declare("platform.lib/name", "Generic Platform Library");
		m->declare("platform.lib/version", "1.3.0");
		m->declare("routes.lib/name", "Faust Signal Routing Library");
		m->declare("routes.lib/version", "1.2.0");
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
		fConst3 = 1.0f / fConst0;
		fConst4 = std::exp(-(16.666666f / fConst0));
		fConst5 = std::exp(-(2e+03f / fConst0));
		fConst6 = std::exp(-(5e+01f / fConst0));
		fConst7 = 1.0f - fConst6;
	}
	
	virtual void instanceResetUserInterface() {
		fHslider0 = FAUSTFLOAT(0.0f);
		fHslider1 = FAUSTFLOAT(6.0f);
		fHslider2 = FAUSTFLOAT(-18.0f);
		fHslider3 = FAUSTFLOAT(1.5e+02f);
		fHslider4 = FAUSTFLOAT(1e+01f);
		fHslider5 = FAUSTFLOAT(4.0f);
		fHslider6 = FAUSTFLOAT(1.0f);
	}
	
	virtual void instanceClear() {
		for (int l0 = 0; l0 < 2; l0 = l0 + 1) {
			fRec1[l0] = 0.0f;
		}
		for (int l1 = 0; l1 < 2; l1 = l1 + 1) {
			fRec2[l1] = 0.0f;
		}
		for (int l2 = 0; l2 < 2; l2 = l2 + 1) {
			fRec3[l2] = 0.0f;
		}
		for (int l3 = 0; l3 < 2; l3 = l3 + 1) {
			fRec5[l3] = 0.0f;
		}
		for (int l4 = 0; l4 < 2; l4 = l4 + 1) {
			fRec6[l4] = 0.0f;
		}
		for (int l5 = 0; l5 < 2; l5 = l5 + 1) {
			fRec4[l5] = 0.0f;
		}
		for (int l6 = 0; l6 < 2; l6 = l6 + 1) {
			fRec7[l6] = 0.0f;
		}
		for (int l7 = 0; l7 < 2; l7 = l7 + 1) {
			fRec8[l7] = 0.0f;
		}
		for (int l8 = 0; l8 < 2; l8 = l8 + 1) {
			fRec0[l8] = 0.0f;
		}
		for (int l9 = 0; l9 < 2; l9 = l9 + 1) {
			fRec9[l9] = 0.0f;
		}
		for (int l10 = 0; l10 < 2; l10 = l10 + 1) {
			fRec10[l10] = 0.0f;
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
	
	virtual CompressorDsp* clone() {
		return new CompressorDsp();
	}
	
	virtual int getSampleRate() {
		return fSampleRate;
	}
	
	virtual void buildUserInterface(UI* ui_interface) {
		ui_interface->openVerticalBox("fxbox_compressor");
		ui_interface->addHorizontalSlider("attack", &fHslider4, FAUSTFLOAT(1e+01f), FAUSTFLOAT(0.1f), FAUSTFLOAT(1e+02f), FAUSTFLOAT(0.001f));
		ui_interface->addHorizontalBargraph("gr", &fHbargraph0, FAUSTFLOAT(0.0f), FAUSTFLOAT(6e+01f));
		ui_interface->addHorizontalSlider("knee", &fHslider1, FAUSTFLOAT(6.0f), FAUSTFLOAT(0.0f), FAUSTFLOAT(12.0f), FAUSTFLOAT(0.01f));
		ui_interface->addHorizontalSlider("limiter", &fHslider6, FAUSTFLOAT(1.0f), FAUSTFLOAT(0.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(1.0f));
		ui_interface->addHorizontalSlider("makeup", &fHslider0, FAUSTFLOAT(0.0f), FAUSTFLOAT(0.0f), FAUSTFLOAT(24.0f), FAUSTFLOAT(0.01f));
		ui_interface->addHorizontalSlider("ratio", &fHslider5, FAUSTFLOAT(4.0f), FAUSTFLOAT(1.0f), FAUSTFLOAT(2e+01f), FAUSTFLOAT(0.001f));
		ui_interface->addHorizontalSlider("release", &fHslider3, FAUSTFLOAT(1.5e+02f), FAUSTFLOAT(2e+01f), FAUSTFLOAT(1e+03f), FAUSTFLOAT(0.01f));
		ui_interface->addHorizontalSlider("threshold", &fHslider2, FAUSTFLOAT(-18.0f), FAUSTFLOAT(-4e+01f), FAUSTFLOAT(0.0f), FAUSTFLOAT(0.01f));
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
		float fSlow4 = fConst1 * float(fHslider4);
		float fSlow5 = fConst1 * float(fHslider5);
		float fSlow6 = fConst7 * float(fHslider6);
		for (int i0 = 0; i0 < count; i0 = i0 + 1) {
			fRec1[0] = fSlow0 + fConst2 * fRec1[1];
			float fTemp0 = std::pow(1e+01f, 0.05f * fRec1[0]);
			fRec2[0] = fSlow1 + fConst2 * fRec2[1];
			float fTemp1 = 0.5f * fRec2[0];
			fRec3[0] = fSlow2 + fConst2 * fRec3[1];
			float fTemp2 = fRec3[0] + fTemp1;
			float fTemp3 = float(input0[i0]);
			float fTemp4 = std::fabs(fTemp3);
			fRec5[0] = fSlow3 + fConst2 * fRec5[1];
			float fTemp5 = 0.001f * fRec5[0];
			int iTemp6 = std::fabs(fTemp5) < 1.1920929e-07f;
			float fTemp7 = ((iTemp6) ? 0.0f : std::exp(-(fConst3 / ((iTemp6) ? 1.0f : fTemp5))));
			fRec6[0] = fSlow4 + fConst2 * fRec6[1];
			float fTemp8 = 0.001f * fRec6[0];
			int iTemp9 = std::fabs(fTemp8) < 1.1920929e-07f;
			float fTemp10 = ((iTemp9) ? 0.0f : std::exp(-(fConst3 / ((iTemp9) ? 1.0f : fTemp8))));
			float fTemp11 = ((fTemp4 > fRec4[1]) ? fTemp10 : fTemp7);
			fRec4[0] = fTemp4 * (1.0f - fTemp11) + fRec4[1] * fTemp11;
			float fTemp12 = 2e+01f * std::log10(std::max<float>(1.1754944e-38f, fRec4[0]));
			float fTemp13 = fRec3[0] - fTemp1;
			int iTemp14 = (fTemp12 > fTemp13) + (fTemp12 > fTemp2);
			float fTemp15 = std::max<float>(1.1920929e-07f, fRec2[0]);
			fRec7[0] = fSlow5 + fConst2 * fRec7[1];
			float fTemp16 = 1.0f - 1.0f / std::max<float>(1.0f, fRec7[0]);
			float fTemp17 = float(input1[i0]);
			float fTemp18 = std::fabs(fTemp17);
			float fTemp19 = ((fTemp18 > fRec8[1]) ? fTemp10 : fTemp7);
			fRec8[0] = fTemp18 * (1.0f - fTemp19) + fRec8[1] * fTemp19;
			float fTemp20 = 2e+01f * std::log10(std::max<float>(1.1754944e-38f, fRec8[0]));
			int iTemp21 = (fTemp20 > fTemp13) + (fTemp20 > fTemp2);
			float fTemp22 = std::min<float>(-(fTemp16 * std::max<float>(0.0f, ((iTemp14 == 0) ? 0.0f : ((iTemp14 == 1) ? 0.5f * (CompressorDsp_faustpower2_f(fTemp12 + fTemp1 - fRec3[0]) / fTemp15) : fTemp12 - fRec3[0])))), -(fTemp16 * std::max<float>(0.0f, ((iTemp21 == 0) ? 0.0f : ((iTemp21 == 1) ? 0.5f * (CompressorDsp_faustpower2_f(fTemp1 + fTemp20 - fRec3[0]) / fTemp15) : fTemp20 - fRec3[0])))));
			fHbargraph0 = FAUSTFLOAT(std::max<float>(0.0f, -fTemp22));
			float fTemp23 = std::pow(1e+01f, 0.05f * fTemp22);
			float fTemp24 = fTemp3 * fTemp23 * fTemp0;
			float fTemp25 = std::fabs(fTemp24);
			float fTemp26 = ((fTemp25 > fRec0[1]) ? fConst5 : fConst4);
			fRec0[0] = fTemp25 * (1.0f - fTemp26) + fRec0[1] * fTemp26;
			float fTemp27 = 2e+01f * std::log10(std::max<float>(1.1754944e-38f, fRec0[0]));
			int iTemp28 = (fTemp27 > -1.5f) + (fTemp27 > -0.5f);
			float fTemp29 = fTemp17 * fTemp23 * fTemp0;
			float fTemp30 = std::fabs(fTemp29);
			float fTemp31 = ((fTemp30 > fRec9[1]) ? fConst5 : fConst4);
			fRec9[0] = fTemp30 * (1.0f - fTemp31) + fRec9[1] * fTemp31;
			float fTemp32 = 2e+01f * std::log10(std::max<float>(1.1754944e-38f, fRec9[0]));
			int iTemp33 = (fTemp32 > -1.5f) + (fTemp32 > -0.5f);
			fRec10[0] = fSlow6 + fConst6 * fRec10[1];
			float fTemp34 = 1.0f - fRec10[0] * (1.0f - std::pow(1e+01f, 0.05f * std::min<float>(-(std::max<float>(0.0f, ((iTemp28 == 0) ? 0.0f : ((iTemp28 == 1) ? 0.5f * CompressorDsp_faustpower2_f(fTemp27 + 1.5f) : fTemp27 + 1.0f)))), -(std::max<float>(0.0f, ((iTemp33 == 0) ? 0.0f : ((iTemp33 == 1) ? 0.5f * CompressorDsp_faustpower2_f(fTemp32 + 1.5f) : fTemp32 + 1.0f)))))));
			output0[i0] = FAUSTFLOAT(fTemp24 * fTemp34);
			output1[i0] = FAUSTFLOAT(fTemp29 * fTemp34);
			fRec1[1] = fRec1[0];
			fRec2[1] = fRec2[0];
			fRec3[1] = fRec3[0];
			fRec5[1] = fRec5[0];
			fRec6[1] = fRec6[0];
			fRec4[1] = fRec4[0];
			fRec7[1] = fRec7[0];
			fRec8[1] = fRec8[0];
			fRec0[1] = fRec0[0];
			fRec9[1] = fRec9[0];
			fRec10[1] = fRec10[0];
		}
	}

};
} // namespace fxfaust

#endif
