#define _WIN32_WINNT 0x0501
#include "../SDK/foobar2000.h"
#include "../ATLHelpers/ATLHelpers.h"
#include "resource.h"
#include "Phaser.h"

static void RunConfigPopup( const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback );
class dsp_phaser : public dsp_impl_base
{
	int m_rate, m_ch, m_ch_mask;
	float freq; //0.1 - 4.0
	float startphase;  //0 - 360
	float fb; //-100 - 100
	int depth;//0-255
	int stages; //2-24
	int drywet; //0-255
	pfc::array_t<Phaser> m_buffers;
public:
	static GUID g_get_guid()
	{
		// {8B54D803-EFEA-4b6c-B3BE-921F6ADC7221}
		static const GUID guid = 
		{ 0x8b54d803, 0xefea, 0x4b6c, { 0xb3, 0xbe, 0x92, 0x1f, 0x6a, 0xdc, 0x72, 0x21 } };
		return guid;
	}

	dsp_phaser( dsp_preset const & in ) : m_rate( 0 ), m_ch( 0 ), m_ch_mask( 0 ), freq(0.4),startphase(0),fb(0),depth(100),stages(2),drywet(128)
	{
		parse_preset( freq,startphase,fb,depth,stages,drywet, in );
	}

	static void g_get_name( pfc::string_base & p_out ) { p_out = "Phaser"; }

	bool on_chunk( audio_chunk * chunk, abort_callback & )
	{
		if ( chunk->get_srate() != m_rate || chunk->get_channels() != m_ch || chunk->get_channel_config() != m_ch_mask )
		{
			m_rate = chunk->get_srate();
			m_ch = chunk->get_channels();
			m_ch_mask = chunk->get_channel_config();
			m_buffers.set_count( 0 );
			m_buffers.set_count( m_ch );
			for ( unsigned i = 0; i < m_ch; i++ )
			{
				Phaser & e = m_buffers[ i ];
				e.SetDepth(depth);
				e.SetDryWet(drywet);
				e.SetFeedback(fb);
				e.SetLFOFreq(freq);
				e.SetLFOStartPhase(startphase);
				e.SetStages(stages);
				e.init(m_rate);
			}
		}

		for ( unsigned i = 0; i < m_ch; i++ )
		{
			Phaser & e = m_buffers[ i ];
			audio_sample * data = chunk->get_data() + i;
			for ( unsigned j = 0, k = chunk->get_sample_count(); j < k; j++ )
			{
				*data = e.Process(*data);
				data += m_ch;
			}
		}

		return true;
	}

	void on_endofplayback( abort_callback & ) { }
	void on_endoftrack( abort_callback & ) { }

	void flush()
	{
		m_buffers.set_count( 0 );
		m_rate = 0;
		m_ch = 0;
		m_ch_mask = 0;
	}

	double get_latency()
	{
		return 0;
	}

	bool need_track_change_mark()
	{
		return false;
	}

	static bool g_get_default_preset( dsp_preset & p_out )
	{
		make_preset(0.4,0,0,100,2,128, p_out );
		return true;
	}
	static void g_show_config_popup( const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback )
	{
		::RunConfigPopup( p_data, p_parent, p_callback );
	}
	static bool g_have_config_popup() { return true; }
	static void make_preset( float freq,float startphase,float fb,int depth,int stages,int drywet, dsp_preset & out )
	{
		dsp_preset_builder builder; 
		builder << freq; 
		builder << startphase;
		builder << fb;
		builder << depth;
		builder << stages;
		builder << drywet;
		builder.finish( g_get_guid(), out );
	}                        
	static void parse_preset(float & freq,float & startphase,float & fb,int & depth,int & stages,int  & drywet, const dsp_preset & in)
	{
		try
		{
			dsp_preset_parser parser(in);
			parser >> freq; 
			parser >> startphase;
			parser >> fb;
			parser >> depth;
			parser >> stages;
			parser >> drywet;
		}
		catch(exception_io_data) {freq= 0.4;startphase=0;fb=0;depth=100;stages=2;drywet=128;}
	}
};

class CMyDSPPopupPhaser : public CDialogImpl<CMyDSPPopupPhaser>
{
public:
	CMyDSPPopupPhaser( const dsp_preset & initData, dsp_preset_edit_callback & callback ) : m_initData( initData ), m_callback( callback ) { }
	enum { IDD = IDD_PHASER };

	enum
	{
		FreqMin = 1,
		FreqMax = 200,
		FreqRangeTotal = FreqMax - FreqMin,
		StartPhaseMin = 0,
		StartPhaseMax = 360,
		StartPhaseTotal =  360,
		FeedbackMin = -100,
		FeedbackMax = 100,
		FeedbackRangeTotal = 200,
		DepthMin = 0,
		DepthMax = 255,
		DepthRangeTotal = 255,
		StagesMin = 1,
		StagesMax = 24,
		StagesRangeTotal = 24,
		DryWetMin = 0,
		DryWetMax = 255,
		DryWetRangeTotal = 255
	};

	BEGIN_MSG_MAP( CMyDSPPopup )
		MSG_WM_INITDIALOG( OnInitDialog )
		COMMAND_HANDLER_EX( IDOK, BN_CLICKED, OnButton )
		COMMAND_HANDLER_EX( IDCANCEL, BN_CLICKED, OnButton )
		MSG_WM_HSCROLL( OnHScroll )
	END_MSG_MAP()

private:

	BOOL OnInitDialog(CWindow, LPARAM)
	{
		slider_freq = GetDlgItem(IDC_PHASERSLFOFREQ);
		slider_freq.SetRange(FreqMin,FreqMax);
		slider_startphase = GetDlgItem(IDC_PHASERSLFOSTARTPHASE);
		slider_startphase.SetRange(0,StartPhaseMax);
		slider_fb = GetDlgItem(IDC_PHASERSFEEDBACK);
		slider_fb.SetRange(-100,100);
		slider_depth = GetDlgItem(IDC_PHASERSDEPTH);
		slider_depth.SetRange(0,255);
		slider_stages =GetDlgItem(IDC_PHASERSTAGES);
		slider_stages.SetRange(StagesMin,StagesMax);
		slider_drywet = GetDlgItem(IDC_PHASERSDRYWET);
		slider_drywet.SetRange(0,255);
		{
			float freq; //0.1 - 4.0
			float startphase;  //0 - 360
			float fb; //-100 - 100
			int depth;//0-255
			int stages; //2-24
			int drywet; //0-255  
			dsp_phaser::parse_preset( freq,startphase,fb,depth,stages,drywet, m_initData);
			slider_freq.SetPos((double)(10*freq));
			slider_startphase.SetPos(startphase);
			slider_fb.SetPos(fb);
			slider_depth.SetPos(depth);
			slider_stages.SetPos(stages);
			slider_drywet.SetPos(drywet);
			RefreshLabel(freq,startphase,fb,depth,stages,drywet);

		}
		
		return TRUE;
	}

	void OnButton( UINT, int id, CWindow )
	{
		EndDialog( id );
	}

	void OnHScroll( UINT nSBCode, UINT nPos, CScrollBar pScrollBar )
	{
		float freq; 
		float startphase;  
		float fb; 
		int depth;
		int stages;
		int drywet;

		freq = slider_freq.GetPos()/10.0;
		startphase = slider_startphase.GetPos();
		fb = slider_fb.GetPos();
		depth = slider_depth.GetPos();
		stages = slider_stages.GetPos();
		drywet = slider_drywet.GetPos();
		{
			dsp_preset_impl preset;
			dsp_phaser::make_preset( freq,startphase,fb,depth,stages,drywet, preset );
			m_callback.on_preset_changed( preset );
		}
		RefreshLabel(freq,startphase,fb,depth,stages,drywet);
		
	}

	void RefreshLabel( float freq,float startphase,float fb,int depth,int stages,int drywet  )
	{
		pfc::string_formatter msg; 
		msg << "LFO Frequency: ";
		msg << pfc::format_float( freq,0,1 ) << " Hz";
		::uSetDlgItemText( *this, IDC_PHASERSLFOFREQINFO, msg );
		msg.reset();
		msg << "LFO Start Phase : ";
		msg << pfc::format_int( startphase ) << " (.deg)";
		::uSetDlgItemText( *this, IDC_PHASERSLFOSTARTPHASEINFO, msg );
		msg.reset();
		msg << "Feedback: ";
		msg << pfc::format_int( fb ) << "%";
		::uSetDlgItemText( *this, IDC_PHASERSFEEDBACKINFO, msg );
		msg.reset();
		msg << "Depth: ";
		msg << pfc::format_int( depth ) << "";
		::uSetDlgItemText( *this, IDC_PHASERSDEPTHINFO, msg );
		msg.reset();
		msg << "Stages: ";
		msg << pfc::format_int( stages ) << "";
		::uSetDlgItemText( *this, IDC_PHASERSTAGESINFO, msg );
		msg.reset();
		msg << "Dry/Wet: ";
		msg << pfc::format_int( drywet ) << "";
		::uSetDlgItemText( *this, IDC_PHASERDRYWETINFO, msg );

	}

	const dsp_preset & m_initData; // modal dialog so we can reference this caller-owned object.
	dsp_preset_edit_callback & m_callback;

	CTrackBarCtrl slider_freq, slider_startphase,slider_fb,slider_depth,slider_stages,slider_drywet;
};

static void RunConfigPopup( const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback )
{
	CMyDSPPopupPhaser popup( p_data, p_callback );
	if ( popup.DoModal(p_parent) != IDOK ) p_callback.on_preset_changed( p_data );
}

static dsp_factory_t<dsp_phaser> g_dsp_phaser_factory;