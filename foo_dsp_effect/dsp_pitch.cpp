#define _WIN32_WINNT 0x0501
#include "../SDK/foobar2000.h"
#include "../ATLHelpers/ATLHelpers.h"
#include "resource.h"
#include "SoundTouch/SoundTouch.h"
#include "SoundTouch/FIFOSampleBuffer.h"
using namespace soundtouch;

static void RunDSPConfigPopup( const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback );
#define BUFFER_SIZE 2048
class dsp_pitch : public dsp_impl_base
{
	SoundTouch * p_soundtouch;
	int m_rate, m_ch, m_ch_mask;
	float pitch_amount;
	pfc::array_t<soundtouch::SAMPLETYPE>samplebuf;
	unsigned buffered;
private:
	void insert_chunks()
	{
		uint samples = p_soundtouch->numSamples();
		if (!samples) return;
		samplebuf.grow_size(BUFFER_SIZE * m_ch);
		soundtouch::SAMPLETYPE * src = samplebuf.get_ptr();
		do
		{
			samples = p_soundtouch->receiveSamples(src, BUFFER_SIZE);
			if (samples > 0)
			{
				audio_chunk * chunk = insert_chunk(samples * m_ch);
				chunk->set_data_32(src, samples, m_ch, m_rate);
			}
		}
		while (samples != 0);
	}

public:
	dsp_pitch( dsp_preset const & in ) : pitch_amount(0.00), m_rate( 0 ), m_ch( 0 ), m_ch_mask( 0 )
	{
		p_soundtouch=0;
		buffered=0;
		parse_preset( pitch_amount, in );
	}
	~dsp_pitch(){
		if (p_soundtouch) delete p_soundtouch;
	}

	// Every DSP type is identified by a GUID.
	static GUID g_get_guid() {
		// Create these with guidgen.exe.
		// {A7FBA855-56D4-46AC-8116-8B2A8DF2FB34}
		static const GUID guid = 
		{ 0xa7fba855, 0x56d4, 0x46ac, { 0x81, 0x16, 0x8b, 0x2a, 0x8d, 0xf2, 0xfb, 0x34 } };
		return guid;
	}

	// We also need a name, so the user can identify the DSP.
	// The name we use here does not describe what the DSP does,
	// so it would be a bad name. We can excuse this, because it
	// doesn't do anything useful anyway.
	static void g_get_name(pfc::string_base & p_out) {
		p_out = "Pitch Shift";
	}

	virtual void on_endoftrack(abort_callback & p_abort) {
		// This method is called when a track ends.
		// We need to do the same thing as flush(), so we just call it.
		
	}

	virtual void on_endofplayback(abort_callback & p_abort) {
		// This method is called on end of playback instead of flush().
		// We need to do the same thing as flush(), so we just call it.
		if (p_soundtouch)
		{
			insert_chunks();
			if (buffered)
			{
				p_soundtouch->putSamples(samplebuf.get_ptr(), buffered);
				buffered = 0;
			}
			p_soundtouch->flush();
			insert_chunks();
			delete p_soundtouch;
			p_soundtouch = 0;
		}
	}

	// The framework feeds input to our DSP using this method.
	// Each chunk contains a number of samples with the same
	// stream characteristics, i.e. same sample rate, channel count
	// and channel configuration.
	virtual bool on_chunk(audio_chunk * chunk, abort_callback & p_abort) {
		t_size sample_count = chunk->get_sample_count();
		audio_sample * src = chunk->get_data();

		if ( chunk->get_srate() != m_rate || chunk->get_channels() != m_ch || chunk->get_channel_config() != m_ch_mask )
		{
			m_rate = chunk->get_srate();
			m_ch = chunk->get_channels();
			m_ch_mask = chunk->get_channel_config();
			p_soundtouch = new SoundTouch;
			if (!p_soundtouch) return 0;


			p_soundtouch->setSampleRate(m_rate);
			p_soundtouch->setChannels(m_ch);
			p_soundtouch->setPitchSemiTones(pitch_amount);
			bool usequickseek = false;
			bool useaafilter = false; //seems clearer without it
			p_soundtouch->setSetting(SETTING_USE_QUICKSEEK, usequickseek);
			p_soundtouch->setSetting(SETTING_USE_AA_FILTER, useaafilter);


		}
		soundtouch::SAMPLETYPE * dst;
		samplebuf.grow_size(BUFFER_SIZE*m_ch);
		while (sample_count)
		{	
			
			unsigned todo = BUFFER_SIZE - buffered;
			if (todo > sample_count) todo = sample_count;
			dst = samplebuf.get_ptr() + buffered * m_ch;
			for (unsigned i = 0, j = todo * m_ch; i < j; i++)
			{
				*dst++ = (soundtouch::SAMPLETYPE) (*src++);
	    	}
			sample_count -= todo;
			buffered += todo;
			if (buffered == BUFFER_SIZE)
			{
				p_soundtouch->putSamples(samplebuf.get_ptr(), buffered);
				buffered = 0;
				insert_chunks();
			}
		}
		return false;
	}

	virtual void flush() {
		if (p_soundtouch){
			p_soundtouch->clear();
		}
		m_rate = 0;
		m_ch = 0;
		m_ch_mask = 0;
	}

	virtual double get_latency() {
		return (p_soundtouch && m_rate) ? ((double)(p_soundtouch->numSamples() + buffered) / (double)m_rate) : 0;
	}

	virtual bool need_track_change_mark() {
		return false;
	}

	static bool g_get_default_preset( dsp_preset & p_out )
	{
		make_preset( 0.0, p_out );
		return true;
	}
	static void g_show_config_popup( const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback )
	{
		::RunDSPConfigPopup( p_data, p_parent, p_callback );
	}
	static bool g_have_config_popup() { return true; }
	static void make_preset( float pitch, dsp_preset & out )
	{
		dsp_preset_builder builder; 
		builder << pitch; 
		builder.finish( g_get_guid(), out );
	}                        
	static void parse_preset(float & pitch, const dsp_preset & in)
	{
		try
		{
			dsp_preset_parser parser(in);
			parser >> pitch; 
		}
		catch(exception_io_data) {pitch = 0.0;}
	}
};


class CMyDSPPopupPitch : public CDialogImpl<CMyDSPPopupPitch>
{
public:
	CMyDSPPopupPitch( const dsp_preset & initData, dsp_preset_edit_callback & callback ) : m_initData( initData ), m_callback( callback ) { }
	enum { IDD = IDD_PITCH };
	enum
	{
		pitchmin = 0,
		pitchmax = 48
		
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
		slider_drytime = GetDlgItem(IDC_PITCH);
		slider_drytime.SetRange(0, pitchmax);

		{
			float  pitch;
			dsp_pitch::parse_preset(pitch, m_initData);
			slider_drytime.SetPos( (double)(pitch+24));
			RefreshLabel( pitch);
		}
		return TRUE;
	}

	void OnButton( UINT, int id, CWindow )
	{
		EndDialog( id );
	}

	void OnHScroll( UINT nSBCode, UINT nPos, CScrollBar pScrollBar )
	{
		float pitch;
		pitch = slider_drytime.GetPos()-24;
		{
			dsp_preset_impl preset;
			dsp_pitch::make_preset(pitch, preset );
			m_callback.on_preset_changed( preset );
		}
		RefreshLabel( pitch);
	}

	void RefreshLabel(float  pitch )
	{
		pfc::string_formatter msg; 
		msg << "Pitch: ";
		msg << (pitch < 0 ? "" : "+");
		msg << pfc::format_int( pitch) << " semitones";
		::uSetDlgItemText( *this, IDC_PITCHINFO, msg );
		msg.reset();
	}
	const dsp_preset & m_initData; // modal dialog so we can reference this caller-owned object.
	dsp_preset_edit_callback & m_callback;
	CTrackBarCtrl slider_drytime,slider_wettime,slider_dampness,slider_roomwidth,slider_roomsize;
};
static void RunDSPConfigPopup( const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback )
{
	CMyDSPPopupPitch popup( p_data, p_callback );
	if ( popup.DoModal(p_parent) != IDOK ) p_callback.on_preset_changed( p_data );
}

static dsp_factory_t<dsp_pitch> g_dsp_pitch_factory;