#define _WIN32_WINNT 0x0501
#include "../SDK/foobar2000.h"
#include "iirfilters.h"

class deemph_postprocessor_instance : public decode_postprocessor_instance
{
	pfc::array_t<IIRFilter> filter;
	unsigned srate, nch, channel_config;
	bool enabled;
private:
	void init()
	{
		filter.set_size( nch );
		for ( unsigned h = 0; h < nch; h++ )
		{
			//these do not matter, only samplerate does
			filter[ h ].setFrequency(400);
			filter[ h ].setQuality(0);
			filter[ h ].setGain(0);
			filter[ h ].init(srate,RIAA_CD);
		}
	}

	void cleanup()
	{
		filter.set_size( 0 );
		srate = 0;
		nch = 0;
		channel_config = 0;
	}
public:
	deemph_postprocessor_instance()
	{
		cleanup();
	}

	~deemph_postprocessor_instance()
	{
		cleanup();
	}

	virtual bool run( dsp_chunk_list & p_chunk_list, t_uint32 p_flags, abort_callback & p_abort )
	{

		for ( unsigned i = 0; i < p_chunk_list.get_count(); )
		{
			audio_chunk * chunk = p_chunk_list.get_item( i );

			if ( srate != chunk->get_sample_rate() || nch != chunk->get_channels() || channel_config != chunk->get_channel_config() )
			{
				srate = chunk->get_sample_rate();
				nch = chunk->get_channels();
				channel_config = chunk->get_channel_config();
				init();
			}

			if (srate != 44100) return false;

			for ( unsigned k = 0; k < nch; k++ )
			{
				audio_sample * data = chunk->get_data() + k;
				for ( unsigned j = 0, l = chunk->get_sample_count(); j < l; j++ )
				{
					*data = filter[k].Process(*data);
					data += nch;
				}
			}
			i++;
		}
		return true;
	}

	virtual bool get_dynamic_info( file_info & p_out )
	{
		return false;
	}

	virtual void flush()
	{
		cleanup();
	}

	virtual double get_buffer_ahead()
	{
		return 0;
	}
};


class deemph_postprocessor_entry : public decode_postprocessor_entry
{
public:
	virtual bool instantiate( const file_info & info, decode_postprocessor_instance::ptr & out )
	{
		unsigned enable_deemp = info.meta_find("PRE_EMPHASIS");
		if (enable_deemp == pfc_infinite){
		return false;
		}
		else{
		const char* enabled = info.meta_enum_value(enable_deemp,0);
		int que = pfc::stricmp_ascii(enabled, "1");
		if (!que){
		console::print("Pre-emphasis detected and enabled in track. Running filter");
		out = new service_impl_t<deemph_postprocessor_instance>;
		return true;
		}
		else
		{
		console::print("Pre-emphasis not enabled.");
		return false;
		}	
		}
	}
};
static service_factory_single_t<deemph_postprocessor_entry> g_deemph_postprocessor_entry_factory;




