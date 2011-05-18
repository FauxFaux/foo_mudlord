#include "../SDK/foobar2000.h"
#include "SoundTouch/SoundTouch.h"

#define MYVERSION "0.8"

static pfc::string_formatter g_get_component_about()
{
	pfc::string_formatter about;
	about << "A special effect DSP for foobar2000 1.1 ->\n";
	about << "Written by mudlord.\n";
	about << "Portions by Jon Watte, Jezar Wakefield, Chris Moeller, Gian-Carlo Pascutto.\n";
	about << "\nUsing SoundTouch library version "<< SOUNDTOUCH_VERSION << "\n";
	about << "SoundTouch (c) Olli Parviainen\n";

	return about;
}


DECLARE_COMPONENT_VERSION_COPY(
"Effect DSP",
	MYVERSION,
	g_get_component_about()
	);
VALIDATE_COMPONENT_FILENAME("foo_dsp_effect.dll");