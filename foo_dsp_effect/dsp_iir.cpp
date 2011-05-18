#define _WIN32_WINNT 0x0501
#include "../SDK/foobar2000.h"
#include "../ATLHelpers/ATLHelpers.h"
#include "resource.h"
#include "iirfilters.h"

static void RunConfigPopup( const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback );

class dsp_iir : public dsp_impl_base
{
	int m_rate, m_ch, m_ch_mask;
	int p_freq; //40.0, 13000.0 (Frequency: Hz)
	int p_gain; //gain
	int p_type; //filter type
	pfc::array_t<IIRFilter> m_buffers;
public:
	static GUID g_get_guid()
	{
		// {FEA092A6-EA54-4f62-B180-4C88B9EB2B67}
		static const GUID guid = 
		{ 0xfea092a6, 0xea54, 0x4f62, { 0xb1, 0x80, 0x4c, 0x88, 0xb9, 0xeb, 0x2b, 0x67 } };
		return guid;
	}

	dsp_iir( dsp_preset const & in ) :m_rate( 0 ), m_ch( 0 ), m_ch_mask( 0 ), p_freq(400), p_gain(10), p_type(0)
	{
		parse_preset( p_freq,p_gain,p_type, in );
	}

	static void g_get_name( pfc::string_base & p_out ) { p_out = "IIR Filter"; }

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
				IIRFilter & e = m_buffers[ i ];
				e.setFrequency(p_freq);
				e.setQuality(0.707);
				e.setGain(p_gain);
				e.init(m_rate,p_type);
			}
		}

		for ( unsigned i = 0; i < m_ch; i++ )
		{
			IIRFilter & e = m_buffers[ i ];
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
		make_preset(400, 10, 1, p_out );
		return true;
	}
	static void g_show_config_popup( const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback )
	{
		::RunConfigPopup( p_data, p_parent, p_callback );
	}
	static bool g_have_config_popup() { return true; }

	static void make_preset( int p_freq,int p_gain,int p_type, dsp_preset & out )
	{
		dsp_preset_builder builder; 
		builder << p_freq; 
		builder << p_gain; //gain
		builder << p_type; //filter type
		builder.finish( g_get_guid(), out );
	}                        
	static void parse_preset(int & p_freq,int & p_gain,int & p_type, const dsp_preset & in)
	{
		try
		{
			dsp_preset_parser parser(in);
			parser >> p_freq; 
			parser >> p_gain; //gain
			parser >> p_type; //filter type
		}
		catch(exception_io_data) { p_freq=400; p_gain=10;p_type=1;}
	}
};

class CMyDSPPopupIIR : public CDialogImpl<CMyDSPPopupIIR>
{
public:
	CMyDSPPopupIIR( const dsp_preset & initData, dsp_preset_edit_callback & callback ) : m_initData( initData ), m_callback( callback ) { }
	enum { IDD = IDD_IIR };

	enum
	{
		FreqMin = 0,
		FreqMax = 20000,
		FreqRangeTotal = FreqMax,
		GainMin = -100,
		GainMax = 100,
		GainRangeTotal= GainMax - GainMin
	};

	BEGIN_MSG_MAP( CMyDSPPopup )
		MSG_WM_INITDIALOG( OnInitDialog )
		COMMAND_HANDLER_EX( IDOK, BN_CLICKED, OnButton )
		COMMAND_HANDLER_EX( IDCANCEL, BN_CLICKED, OnButton )
		COMMAND_HANDLER_EX(IDC_IIRTYPE, CBN_SELCHANGE, OnChange)
		MSG_WM_HSCROLL( OnChange )
	END_MSG_MAP()

private:

	BOOL OnInitDialog(CWindow, LPARAM)
	{
		CWindow w;

		slider_freq = GetDlgItem(IDC_IIRFREQ);
		slider_freq.SetRange(0, FreqMax);
		slider_gain = GetDlgItem(IDC_IIRGAIN);
		slider_gain.SetRange(GainMin,GainMax);
		{
			int p_freq; 
			int p_gain; 
			int p_type; 
			dsp_iir::parse_preset( p_freq,p_gain,p_type, m_initData );
			if (p_type <= 7)
			{
				slider_gain.EnableWindow(FALSE);
				slider_freq.EnableWindow(TRUE);
				if (p_type == 7) slider_freq.EnableWindow(FALSE);
			}
			else
			{
				slider_freq.EnableWindow(TRUE);
				slider_gain.EnableWindow(TRUE);
			}
			slider_freq.SetPos(p_freq );
			slider_gain.SetPos(p_gain);
			w = GetDlgItem(IDC_IIRTYPE);
			uSendMessageText(w, CB_ADDSTRING, 0, "Resonant Lowpass");
			uSendMessageText(w, CB_ADDSTRING, 0, "Resonant Highpass");
			uSendMessageText(w, CB_ADDSTRING, 0, "Bandpass (CSG)");
			uSendMessageText(w, CB_ADDSTRING, 0, "Bandpass (ZPG)");
			uSendMessageText(w, CB_ADDSTRING, 0, "Allpass");
			uSendMessageText(w, CB_ADDSTRING, 0, "Notch");
			uSendMessageText(w, CB_ADDSTRING, 0, "RIAA Tape/Vinyl De-emphasis");
			uSendMessageText(w, CB_ADDSTRING, 0, "CD De-emphasis");
			uSendMessageText(w, CB_ADDSTRING, 0, "Parametric EQ (single band)");
			uSendMessageText(w, CB_ADDSTRING, 0, "Bass Boost");
			uSendMessageText(w, CB_ADDSTRING, 0, "Low shelf");
			uSendMessageText(w, CB_ADDSTRING, 0, "High shelf");
			::SendMessage(w, CB_SETCURSEL, p_type, 0);
			RefreshLabel(  p_freq,p_gain,p_type);

		}
		return TRUE;
	}

	void OnButton( UINT, int id, CWindow )
	{
		EndDialog( id );
	}

	void OnChange( UINT, int id, CWindow )
	{
		CWindow w;
		int p_freq; //40.0, 13000.0 (Frequency: Hz)
		int p_gain; //gain
		int p_type; //filter type
		p_freq = slider_freq.GetPos();
		p_gain = slider_gain.GetPos();
		p_type = SendDlgItemMessage( IDC_IIRTYPE, CB_GETCURSEL );
		{
			dsp_preset_impl preset;
			dsp_iir::make_preset( p_freq,p_gain,p_type, preset );
			m_callback.on_preset_changed( preset );
		}
		if (p_type <= 7)
		{
			slider_gain.EnableWindow(FALSE);
			slider_freq.EnableWindow(TRUE);
			if (p_type == 7) slider_freq.EnableWindow(FALSE);
		}
		else
		{
			slider_freq.EnableWindow(TRUE);
			slider_gain.EnableWindow(TRUE);
		}
		RefreshLabel(  p_freq,p_gain, p_type);
		
	}

	void RefreshLabel( int p_freq,int p_gain, int p_type)
	{
		pfc::string_formatter msg; 
		if (p_type == 7)
		{
			msg << "Frequency: disabled";
		}
		else
		{
			msg << "Frequency: ";
			msg << pfc::format_int(  p_freq ) << " Hz";
		}
		::uSetDlgItemText( *this, IDC_IIRFREQINFO, msg );
		msg.reset();
		if (p_type <= 7)
		{
			msg << "Gain: disabled";
		}
		else
		{
			msg << "Gain: ";
			msg << pfc::format_int(  p_gain) << " db";
		}
		::uSetDlgItemText( *this, IDC_IIRGAININFO, msg );
	}

	const dsp_preset & m_initData; // modal dialog so we can reference this caller-owned object.
	dsp_preset_edit_callback & m_callback;
	CTrackBarCtrl slider_freq, slider_gain;
};

static void RunConfigPopup( const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback )
{
	CMyDSPPopupIIR popup( p_data, p_callback );
	if ( popup.DoModal(p_parent) != IDOK ) p_callback.on_preset_changed( p_data );
}


static dsp_factory_t<dsp_iir> g_dsp_iir_factory;