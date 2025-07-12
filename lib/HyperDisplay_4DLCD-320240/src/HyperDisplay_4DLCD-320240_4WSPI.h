//
// 4DLCD-320240 Display Driver Library
// A standalone driver for the 4DLCD-320240 2.4" TFT display
// Merged from HyperDisplay_ILI9341 library
//

#ifndef _HYPERDISPLAY_4DLCD_320240_4WSPI_H_
#define _HYPERDISPLAY_4DLCD_320240_4WSPI_H_

////////////////////////////////////////////////////////////
//							Includes    				  //
////////////////////////////////////////////////////////////
#include "hyperdisplay.h"		// Inherit drawing functions from this library
#include "fast_hsv2rgb.h"		// Used to work with HSV color space		
#include <SPI.h>				// Arduino SPI support

////////////////////////////////////////////////////////////
//							Defines     				  //
////////////////////////////////////////////////////////////
#define LCD320240_WIDTH 		240
#define LCD320240_HEIGHT 		320
#define LCD320240_START_ROW		0
#define LCD320240_START_COL		0
#define LCD320240_STOP_ROW 		319
#define LCD320240_STOP_COL		239

#define LCD320240_MAX_X 240
#define LCD320240_MAX_Y 320
#define LCD320240_MAX_BPP 4

// SPI Configuration
#define LCD320240_SPI_DATA_ORDER MSBFIRST
#define LCD320240_SPI_MODE SPI_MODE0
#define LCD320240_SPI_DEFAULT_FREQ 24000000
#define LCD320240_SPI_MAX_FREQ 	32000000

////////////////////////////////////////////////////////////
//							Typedefs    				  //
////////////////////////////////////////////////////////////
typedef enum{
	LCD320240_STAT_Nominal = 0x00,
	LCD320240_STAT_Error
}LCD320240_STAT_t;

typedef enum{
	LCD320240_CMD_NOP = 0x00,
	LCD320240_CMD_SWRST,
	//
	LCD320240_CMD_RDIDI = 0x04,
	//
	LCD320240_CMD_RDSTS = 0x09,
	LCD320240_CMD_RDPWR,
	LCD320240_CMD_RDMADCTL,
	LCD320240_CMD_RDPXFMT,
	LCD320240_CMD_RDIM,
	LCD320240_CMD_RDSM,
	LCD320240_CMD_RDSM2,
	LCD320240_CMD_SLPIN,
	LCD320240_CMD_SLPOUT,
	LCD320240_CMD_PTLON,
	LCD320240_CMD_NMLON,
	// 
	LCD320240_CMD_INVOFF = 0x20,
	LCD320240_CMD_INVON,
	//
	LCD320240_CMD_GAMST = 0x26,
	//
	LCD320240_CMD_OFF = 0x28,
	LCD320240_CMD_ON,
	LCD320240_CMD_CASET,
	LCD320240_CMD_RASET,
	LCD320240_CMD_WRRAM,
	LCD320240_CMD_WRCS,
	LCD320240_CMD_PTLAREA = 0x30,
	//
	LCD320240_CMD_WRVSCRL = 0x33,
	LCD320240_CMD_TELOFF,
	LCD320240_CMD_TELON,
	LCD320240_CMD_WRMADCTL,
	LCD320240_CMD_WRVSSA,
	LCD320240_CMD_IDLOFF,
	LCD320240_CMD_IDLON,
	LCD320240_CMD_WRPXFMT,
	//
	LCD320240_CMD_WRNMLFRCTL = 0xB1,
	LCD320240_CMD_WRIDLFRCTL,
	LCD320240_CMD_WRPTLFRCTL,
	LCD320240_CMD_WRDICTL,
	LCD320240_CMD_WRIBPS,
	LCD320240_CMD_WRDF,
	LCD320240_CMD_WRSDRVDIR,
	LCD320240_CMD_WRGDRVDIR,
	//
	LCD320240_CMD_WRPWCTL1 = 0xC0,
	LCD320240_CMD_WRPWCTL2,
	LCD320240_CMD_WRPWCTL3,
	LCD320240_CMD_WRPWCTL4,
	LCD320240_CMD_WRPWCTL5,
	LCD320240_CMD_WRVCOMCTL1,
	LCD320240_CMD_WRVCOMCTL2,
	LCD320240_CMD_WRVCMOFSTCTL,
	//
	LCD320240_CMD_WRID4 = 0xD3,
	//
	LCD320240_CMD_WRNVMFCTL1 = 0xD5,
	LCD320240_CMD_WRNVMFCTL2,
	LCD320240_CMD_WRNVMFCTL3,
	//
	LCD320240_CMD_RDID1 = 0xDA,
	LCD320240_CMD_RDID2,
	LCD320240_CMD_RDID3,
	//
	LCD320240_CMD_WRPGCS = 0xE0,
	LCD320240_CMD_WRNGCS,
	//
	LCD320240_CMD_WRGAMRS = 0xF2
}LCD320240_CMD_t;

typedef enum{
	LCD320240_INTFC_4WSPI = 0x00,
	LCD320240_INTFC_3WSPI,
	LCD320240_INTFC_8080
}LCD320240_INTFC_t;

typedef enum{
	LCD320240_PXLFMT_16 = 0x05,
	LCD320240_PXLFMT_18 = 0x06
}LCD320240_PXLFMT_t;

typedef struct LCD320240_color_18{
	uint8_t r;
	uint8_t g;
	uint8_t b;
}LCD320240_color_18_t;

typedef struct LCD320240_color_16{
	uint8_t rgh;	// Red, green high
	uint8_t glb;	// Green low, blue
}LCD320240_color_16_t;

typedef struct LCD320240_color_12{
	uint8_t b0;	// Red, green high
	uint8_t b1;	// Green low, blue
}LCD320240_color_12_t;

////////////////////////////////////////////////////////////
//					 Class Definition   				  //
////////////////////////////////////////////////////////////
class LCD320240_4WSPI : virtual public hyperdisplay{
private:
protected:
	LCD320240_INTFC_t _intfc;
	LCD320240_PXLFMT_t _pxlfmt;
	
	uint8_t _dc, _rst, _cs, _bl;		// Pin definitions
	SPIClass * _spi;			// Which SPI port to use
	SPISettings _spisettings;

	// Pure virtual functions from HyperDisplay Implemented:
	color_t getOffsetColor(color_t base, uint32_t numPixels);
	void 	hwpixel(hd_hw_extent_t x0, hd_hw_extent_t y0, color_t data = NULL, hd_colors_t colorCycleLength = 1, hd_colors_t startColorOffset = 0);
	void	swpixel( hd_extent_t x0, hd_extent_t y0, color_t data = NULL, hd_colors_t colorCycleLength = 1, hd_colors_t startColorOffset = 0);

public:

	LCD320240_4WSPI();	// Constructor

	static LCD320240_color_18_t hsvTo18b( uint16_t h, uint8_t s, uint8_t v );
	static LCD320240_color_16_t hsvTo16b( uint16_t h, uint8_t s, uint8_t v );
	static LCD320240_color_12_t hsvTo12b( uint16_t h, uint8_t s, uint8_t v, uint8_t odd);

	static LCD320240_color_18_t rgbTo18b( uint8_t r, uint8_t g, uint8_t b );
	static LCD320240_color_16_t rgbTo16b( uint8_t r, uint8_t g, uint8_t b );
	static LCD320240_color_12_t rgbTo12b( uint8_t r, uint8_t g, uint8_t b, uint8_t odd);

	// Low-level interface functions:
	LCD320240_STAT_t writePacket(LCD320240_CMD_t* pcmd = NULL, uint8_t* pdata = NULL, uint16_t dlen = 0);
	LCD320240_STAT_t selectDriver( void );
	LCD320240_STAT_t deselectDriver( void );
	LCD320240_STAT_t setSPIFreq( uint32_t freq );
	LCD320240_STAT_t transferSPIbuffer(uint8_t* pdata, size_t count, bool arduinoStillBroken );

	// Some Utility Functions
	uint8_t getBytesPerPixel( void );

	// Basic Control Functions
	LCD320240_STAT_t swReset( void );
	LCD320240_STAT_t sleepIn( void );
	LCD320240_STAT_t sleepOut( void );
	LCD320240_STAT_t partialModeOn( void );
	LCD320240_STAT_t normalDisplayModeOn( void );
	LCD320240_STAT_t setInversion( bool on );
	LCD320240_STAT_t setPower( bool on );
	LCD320240_STAT_t setColumnAddress( uint16_t start, uint16_t end );
	LCD320240_STAT_t setRowAddress( uint16_t start, uint16_t end );
	LCD320240_STAT_t writeToRAM( uint8_t* pdata, uint16_t numBytes );
	
	// Functions to configure the display fully
	LCD320240_STAT_t setMemoryAccessControl( bool mx, bool my, bool mv, bool ml, bool bgr, bool mh );
	LCD320240_STAT_t selectGammaCurve( uint8_t bmNumber );
	LCD320240_STAT_t setPartialArea(uint16_t start, uint16_t end );
	LCD320240_STAT_t setVerticalScrolling( uint16_t tfa, uint16_t vsa, uint16_t bfa );
	LCD320240_STAT_t setVerticalScrollingStartAddress( uint16_t ssa );
	LCD320240_STAT_t setIdleMode( bool on );
	LCD320240_STAT_t setInterfacePixelFormat( uint8_t CTRLintfc );
	LCD320240_STAT_t setTearingEffectLine( bool on );
	LCD320240_STAT_t setNormalFramerate( uint8_t diva, uint8_t vpa );
	LCD320240_STAT_t setIdleFramerate( uint8_t divb, uint8_t vpb );
	LCD320240_STAT_t setPartialFramerate( uint8_t divc, uint8_t vpc );
	LCD320240_STAT_t setPowerControl1( uint8_t vrh, uint8_t vc );
	LCD320240_STAT_t setPowerControl2( uint8_t bt );
	LCD320240_STAT_t setPowerControl3( uint8_t apa );
	LCD320240_STAT_t setPowerControl4( uint8_t apb );
	LCD320240_STAT_t setPowerControl5( uint8_t apc );
	LCD320240_STAT_t setVCOMControl1( uint8_t vmh, uint8_t vml );
	LCD320240_STAT_t setVCOMControl2( uint8_t vma );
	LCD320240_STAT_t setVCOMOffsetControl( bool nVM, uint8_t vmf );
	LCD320240_STAT_t setSrcDriverDir( bool crl );
	LCD320240_STAT_t setGateDriverDir( bool ctb );
	LCD320240_STAT_t setGamRSel( bool gamrsel );
	LCD320240_STAT_t setPositiveGamCorr( uint8_t* gam16byte );
	LCD320240_STAT_t setNegativeGamCorr( uint8_t* gam16byte );

	// Optimized drawing functions
	virtual void 	hwxline(hd_hw_extent_t x0, hd_hw_extent_t y0, hd_hw_extent_t len, color_t data = NULL, hd_colors_t colorCycleLength = 1, hd_colors_t startColorOffset = 0, bool goLeft = false);
	virtual void    hwyline(hd_hw_extent_t x0, hd_hw_extent_t y0, hd_hw_extent_t len, color_t data = NULL, hd_colors_t colorCycleLength = 1, hd_colors_t startColorOffset = 0, bool goUp = false);
	virtual void 	hwfillFromArray(hd_hw_extent_t x0, hd_hw_extent_t y0, hd_hw_extent_t x1, hd_hw_extent_t y1, color_t data = NULL, hd_pixels_t numPixels = 0, bool Vh = false);

	// Display-specific functions
	LCD320240_STAT_t begin(uint8_t dcPin, uint8_t csPin, uint8_t blPin, SPIClass &spiInterface = SPI, uint32_t spiFreq = LCD320240_SPI_DEFAULT_FREQ);
	LCD320240_STAT_t defaultConfigure( void ); // The recommended settings from the datasheet
	void startup( void );		// The default startup for this particular display

	void getCharInfo(uint8_t val, char_info_t  pchar);

	// Some specialized drawing functions
	void clearDisplay( void );

	// Special Functions
	void setWindowDefaults(wind_info_t*  pwindow); // Overrides default implementation in hyperdisplay class
	void setBacklight(uint8_t b);
};

#endif /* _HYPERDISPLAY_4DLCD_320240_4WSPI_H_ */