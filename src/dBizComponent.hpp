#pragma once
#include "app.hpp"
#include "asset.hpp"


namespace rack {


////////////////////
// Knobs
////////////////////




struct SmallKnob : SVGKnob {
	SmallKnob() {
		minAngle = -0.83*M_PI;
		maxAngle = 0.83*M_PI;
	}
};


struct SmallOra : SmallKnob {
	SmallOra() {
		setSVG(SVG::load(assetGlobal("plugins/dBiz/res/knobs/SmallOra.svg")));
	}
};

struct LargeOra : SmallOra {
	LargeOra() {
		box.size = Vec(46, 46);
	}
};	

struct SmallBlu : SmallKnob {
	SmallBlu() {
		setSVG(SVG::load(assetGlobal("plugins/dBiz/res/Knobs/SmallBlu.svg")));
	}
};

struct LargeBlu : SmallBlu {
	LargeBlu() {
		box.size = Vec(46, 46);
	}
};

struct SmallAzz : SmallKnob {
	SmallAzz() {
		setSVG(SVG::load(assetGlobal("plugins/dBiz/res/Knobs/SmallAzz.svg")));
	}
};

struct LargeAzz : SmallAzz {
	LargeAzz() {
		box.size = Vec(46, 46);
	}
};

struct SmallVio : SmallKnob {
	SmallVio() {
		setSVG(SVG::load(assetGlobal("plugins/dBiz/res/Knobs/SmallVio.svg")));
	}
};

struct LargeVio : SmallVio {
	LargeVio() {
		box.size = Vec(46, 46);
	}
};	

struct SmallYel : SmallKnob {
	SmallYel() {
		setSVG(SVG::load(assetGlobal("plugins/dBiz/res/Knobs/SmallYel.svg")));
	}
};

struct LargeYel : SmallYel {
	LargeYel() {
		box.size = Vec(46, 46);
	}
};

struct SmallGre : SmallKnob {
	SmallGre() {
		setSVG(SVG::load(assetGlobal("plugins/dBiz/res/Knobs/SmallGre.svg")));
	}
};

struct LargeGre : SmallGre {
	LargeGre() {
		box.size = Vec(46, 46);
	}
};

struct SmallCre : SmallKnob {
	SmallCre() {
		setSVG(SVG::load(assetGlobal("plugins/dBiz/res/Knobs/SmallCre.svg")));
	}
};

struct LargeCre : SmallCre {
	LargeCre() {
		box.size = Vec(46, 46);
	}
};

struct SmallBla : SmallKnob {
	SmallBla() {
		setSVG(SVG::load(assetGlobal("plugins/dBiz/res/Knobs/SmallBla.svg")));
	}
};

struct LargeBla : SmallBla {
	LargeBla() {
		box.size = Vec(46, 46);
	}
};

// struct SmallKnobSnapKnob : SmallKnob, SnapKnob {};


struct DaviesKnob : SVGKnob {
	DaviesKnob() {
		minAngle = -0.83*M_PI;
		maxAngle = 0.83*M_PI;
		box.size = Vec(15, 15);
	}
};

struct DaviesGre : DaviesKnob {
	DaviesGre() {
		setSVG(SVG::load(assetGlobal("plugins/dBiz/res/Knobs/DaviesGre.svg")));
	}
};
struct LDaviesGre : DaviesGre {
		LDaviesGre() {
		box.size = Vec(46, 46);	
	}
};

struct DaviesAzz : DaviesKnob {
	DaviesAzz() {
		setSVG(SVG::load(assetGlobal("plugins/dBiz/res/Knobs/DaviesAzz.svg")));
	}
};
struct LDaviesAzz : DaviesAzz {
		LDaviesAzz() {
		box.size = Vec(46, 46);	
	}
};

struct DaviesPur : DaviesKnob {
	DaviesPur() {
		setSVG(SVG::load(assetGlobal("plugins/dBiz/res/Knobs/DaviesPur.svg")));
	}
};
struct LDaviesPur : DaviesPur {
		LDaviesPur() {
		box.size = Vec(46, 46);	
	}
};

struct DaviesBlu : DaviesKnob {
	DaviesBlu() {
		setSVG(SVG::load(assetGlobal("plugins/dBiz/res/Knobs/DaviesBlu.svg")));
	}
};
struct LDaviesBlu : DaviesBlu {
		LDaviesBlu() {
		box.size = Vec(46, 46);	
	}
};

struct DaviesRed : DaviesKnob {
	DaviesRed() {
		setSVG(SVG::load(assetGlobal("plugins/dBiz/res/Knobs/DaviesRed.svg")));
	}
};
struct LDaviesRed : DaviesRed {
		LDaviesRed() {
		box.size = Vec(46, 46);	
	}
};

struct DaviesYel : DaviesKnob {
	DaviesYel() {
		setSVG(SVG::load(assetGlobal("plugins/dBiz/res/Knobs/DaviesYel.svg")));
	}
};
struct LDaviesYel : DaviesYel {
		LDaviesYel() {
		box.size = Vec(46, 46);	
	}
};
// struct DaviesKnobSnapKnob : DaviesKnob, SnapKnob {};

struct SlidePot : SVGSlider {
	SlidePot() {
		Vec margin = Vec(3.5, 3.5);
		maxHandlePos = Vec(-1, -2).plus(margin);
		minHandlePos = Vec(-1, 87).plus(margin);
		background->svg = SVG::load(assetGlobal("plugins/dBiz/res/Slider/SlidePot.svg"));
		background->wrap();
		background->box.pos = margin;
		box.size = background->box.size.plus(margin.mult(2));
		handle->svg = SVG::load(assetGlobal("plugins/dBiz/res/Slider/SlidePotHandle.svg"));
		handle->wrap();
	}
};







////////////////////
// Jacks
////////////////////

struct PJ301MRPort : SVGPort {
	PJ301MRPort() {
		background->svg = SVG::load(assetGlobal("plugins/dBiz/res/jack/PJ301MR.svg"));
		background->wrap();
		box.size = background->box.size;
	}
};

struct PJ301MLPort : SVGPort {
	PJ301MLPort() {
		background->svg = SVG::load(assetGlobal("plugins/dBiz/res/jack/PJ301ML.svg"));
		background->wrap();
		box.size = background->box.size;
	}
};

struct PJ301MIPort : SVGPort {
	PJ301MIPort() {
		background->svg = SVG::load(assetGlobal("plugins/dBiz/res/jack/PJ301MA.svg"));
		background->wrap();
		box.size = background->box.size;
	}
};

struct PJ301MOPort : SVGPort {
	PJ301MOPort() {
		background->svg = SVG::load(assetGlobal("plugins/dBiz/res/jack/PJ301MB.svg"));
		background->wrap();
		box.size = background->box.size;
	}
};

struct PJ301MCPort : SVGPort {
	PJ301MCPort() {
		background->svg = SVG::load(assetGlobal("plugins/dBiz/res/jack/PJ301MW.svg"));
		background->wrap();
		box.size = background->box.size;
	}
};

}


