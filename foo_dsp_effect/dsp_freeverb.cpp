#include <math.h>
#define _WIN32_WINNT 0x0501
#include "../SDK/foobar2000.h"
#include "../ATLHelpers/ATLHelpers.h"
#include "resource.h"
#include "freeverb.h"

static void RunDSPConfigPopup( const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback );
class dsp_reverb : public dsp_impl_base
{
	int m_rate, m_ch, m_ch_mask;
	float drytime;
	float wettime;
    float dampness;
	float roomwidth;
	float roomsize;
	pfc::array_t<revmodel> m_buffers;
	public:

	dsp_reverb( dsp_preset const & in ) : drytime(0.43),wettime (0.57),dampness (0.45),roomwidth(0.56),roomsize (0.56), m_rate( 0 ), m_ch( 0 ), m_ch_mask( 0 )
	{
		parse_preset( drytime, wettime,dampness,roomwidth,roomsize, in );
	}
	static GUID g_get_guid()
	{
		// {97C60D5F-3572-4d35-9260-FD0CF5DBA480}
		static const GUID guid = { 0x97c60d5f, 0x3572, 0x4d35, { 0x92, 0x60, 0xfd, 0xc, 0xf5, 0xdb, 0xa4, 0x80 } };
		return guid;
	}
	static void g_get_name( pfc::string_base & p_out ) { p_out = "Reverb"; }
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
				revmodel & e = m_buffers[ i ];
				e.setwet(wettime);
				e.setdry(drytime);
				e.setdamp(dampness);
				e.setroomsize(roomsize);
				e.setwidth(roomwidth);
			}
		}
		for ( unsigned i = 0; i < m_ch; i++ )
		{
			revmodel & e = m_buffers[ i ];
			audio_sample * data = chunk->get_data() + i;
			for ( unsigned j = 0, k = chunk->get_sample_count(); j < k; j++ )
			{
				*data = e.processsample( *data );
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
		make_preset( 0.43, 0.57,0.45,0.56,0.56, p_out );
		return true;
	}
	static void g_show_config_popup( const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback )
	{
		::RunDSPConfigPopup( p_data, p_parent, p_callback );
	}
	static bool g_have_config_popup() { return true; }
	static void make_preset( float drytime,float wettime,float dampness,float roomwidth,float roomsize, dsp_preset & out )
	{
		dsp_preset_builder builder; 
		builder << drytime; 
		builder << wettime;
		builder << dampness;
		builder << roomwidth;
		builder << roomsize;
		builder.finish( g_get_guid(), out );
	}                        
	static void parse_preset(float & drytime, float & wettime,float & dampness,float & roomwidth,float & roomsize, const dsp_preset & in)
	{
		try
		{
			dsp_preset_parser parser(in);
			parser >> drytime; 
			parser >> wettime;
			parser >> dampness;
			parser >> roomwidth;
			parser >> roomsize;
		}
		catch(exception_io_data) {drytime = 0.43; wettime = 0.57; dampness = 0.45;roomwidth = 0.56;roomsize = 0.56;}
	}
};

class CMyDSPPopupReverb : public CDialogImpl<CMyDSPPopupReverb>
{
public:
	CMyDSPPopupReverb( const dsp_preset & initData, dsp_preset_edit_callback & callback ) : m_initData( initData ), m_callback( callback ) { }
	enum { IDD = IDD_REVERB };
	enum
	{
		drytimemin = 0,
		drytimemax = 100,
		drytimetotal = 100,
		wettimemin = 0,
		wettimemax = 100,
		wettimetotal = 100,
		dampnessmin = 0,
		dampnessmax = 100,
		dampnesstotal = 100,
		roomwidthmin = 0,
		roomwidthmax = 100,
		roomwidthtotal = 100,
		roomsizemin = 0,
		roomsizemax = 100,
		roomsizetotal = 100
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
		slider_drytime = GetDlgItem(IDC_DRYTIME);
		slider_drytime.SetRange(0, drytimetotal);
		slider_wettime = GetDlgItem(IDC_WETTIME);
		slider_wettime.SetRange(0, wettimetotal);
		slider_dampness = GetDlgItem(IDC_DAMPING);
		slider_dampness.SetRange(0, dampnesstotal);
		slider_roomwidth = GetDlgItem(IDC_ROOMWIDTH);
		slider_roomwidth.SetRange(0, roomwidthtotal);
		slider_roomsize = GetDlgItem(IDC_ROOMSIZE);
		slider_roomsize.SetRange(0, roomsizetotal);

		{
			float  drytime,wettime,dampness,roomwidth,roomsize;
			dsp_reverb::parse_preset(drytime,wettime,dampness,roomwidth,roomsize, m_initData);

			slider_drytime.SetPos( (double)(100*drytime));
			slider_wettime.SetPos( (double)(100*wettime));
			slider_dampness.SetPos( (double)(100*dampness));
			slider_roomwidth.SetPos( (double)(100*roomwidth));
			slider_roomsize.SetPos( (double)(100*roomsize));

			RefreshLabel( drytime,wettime, dampness, roomwidth,roomsize);

		}
		return TRUE;
	}

	void OnButton( UINT, int id, CWindow )
	{
		EndDialog( id );
	}

	void OnHScroll( UINT nSBCode, UINT nPos, CScrollBar pScrollBar )
	{
		float  drytime,wettime,dampness,roomwidth,roomsize;
        drytime = slider_drytime.GetPos()/100.0;
		wettime = slider_wettime.GetPos()/100.0;
		dampness = slider_dampness.GetPos()/100.0;
		roomwidth = slider_roomwidth.GetPos()/100.0;
		roomsize = slider_roomsize.GetPos()/100.0;
		{
			dsp_preset_impl preset;
			dsp_reverb::make_preset(drytime,wettime,dampness,roomwidth,roomsize, preset );
			m_callback.on_preset_changed( preset );
		}
		RefreshLabel( drytime,wettime, dampness, roomwidth,roomsize);
	}

	void RefreshLabel(float  drytime,float wettime, float dampness, float roomwidth,float roomsize )
	{
		pfc::string_formatter msg; 
		msg << "Dry Time: ";
		msg << pfc::format_int( 100*drytime ) << "%";
		::uSetDlgItemText( *this, IDC_DRYTIMEINFO, msg );
		msg.reset();
		msg << "Wet Time: ";
		msg << pfc::format_int( 100*wettime ) << "%";
		::uSetDlgItemText( *this, IDC_WETTIMEINFO, msg );
		msg.reset();
		msg << "Damping: ";
		msg << pfc::format_int( 100*dampness ) << "%";
		::uSetDlgItemText( *this, IDC_DAMPINGINFO, msg );
		msg.reset();
		msg << "Room Width: ";
		msg << pfc::format_int( 100*roomwidth ) << "%";
		::uSetDlgItemText( *this, IDC_ROOMWIDTHINFO, msg );
		msg.reset();
		msg << "Room Size: ";
		msg << pfc::format_int( 100*roomsize ) << "%";
		::uSetDlgItemText( *this, IDC_ROOMSIZEINFO, msg );
	}
	const dsp_preset & m_initData; // modal dialog so we can reference this caller-owned object.
	dsp_preset_edit_callback & m_callback;
	CTrackBarCtrl slider_drytime,slider_wettime,slider_dampness,slider_roomwidth,slider_roomsize;
};
static void RunDSPConfigPopup( const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback )
{
	CMyDSPPopupReverb popup( p_data, p_callback );
	if ( popup.DoModal(p_parent) != IDOK ) p_callback.on_preset_changed( p_data );
}

static dsp_factory_t<dsp_reverb> g_dsp_reverb_factory;