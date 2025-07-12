#include "HyperDisplay_4DLCD-320240_4WSPI.h"

#define ARDUINO_STILL_BROKEN 1 // Referring to the epic fail that is SPI.transfer(buf, len)

////////////////////////////////////////////////////////////
//					LCD320240_4WSPI Implementation		  //
////////////////////////////////////////////////////////////
char_info_t LCD320240_Default_CharInfo;
wind_info_t LCD320240_Default_Window;

LCD320240_4WSPI::LCD320240_4WSPI() : hyperdisplay(LCD320240_WIDTH, LCD320240_HEIGHT)
{
	_intfc = LCD320240_INTFC_4WSPI;
	SPISettings tempSettings(LCD320240_SPI_MAX_FREQ, LCD320240_SPI_DATA_ORDER, LCD320240_SPI_MODE);
	_spisettings = tempSettings;
}

////////////////////////////////////////////////////////////
//				HSV and RGB Conversion Functions		  //
////////////////////////////////////////////////////////////
LCD320240_color_18_t LCD320240_4WSPI::hsvTo18b( uint16_t h, uint8_t s, uint8_t v ){
	uint8_t r; 
	uint8_t g;
	uint8_t b;
	fast_hsv2rgb_8bit(h, s, v, &r, &g , &b);
	return rgbTo18b( r, g, b );
}

LCD320240_color_16_t LCD320240_4WSPI::hsvTo16b( uint16_t h, uint8_t s, uint8_t v ){
	uint8_t r; 
	uint8_t g;
	uint8_t b;
	fast_hsv2rgb_8bit(h, s, v, &r, &g , &b);
	return rgbTo16b( r, g, b );
}

LCD320240_color_18_t LCD320240_4WSPI::rgbTo18b( uint8_t r, uint8_t g, uint8_t b ){
	LCD320240_color_18_t retval;
	retval.r = r;
	retval.g = g;
	retval.b = b;
	return retval;
}

LCD320240_color_16_t LCD320240_4WSPI::rgbTo16b( uint8_t r, uint8_t g, uint8_t b ){
	LCD320240_color_16_t retval;
	retval.rgh = ((r & 0xF8) | ( g >> 5));
	retval.glb = (((g & 0x1C) << 3) | (b >> 3));
	return retval;
}

uint8_t LCD320240_4WSPI::getBytesPerPixel( void )
{
	uint8_t bpp = 0;
	switch(_pxlfmt)
	{
		case LCD320240_PXLFMT_18 :
			bpp = offsetof( LCD320240_color_18_t, b) + 1;
			break;

		case LCD320240_PXLFMT_16 :
			bpp = offsetof( LCD320240_color_16_t, glb) + 1;
			break;

		default :
			break;
	}
	return bpp;
}

////////////////////////////////////////////////////////////
//		Pure virtual functions from HyperDisplay		  //
////////////////////////////////////////////////////////////
color_t LCD320240_4WSPI::getOffsetColor(color_t base, uint32_t numPixels)
{
	switch(_pxlfmt)
	{
		case LCD320240_PXLFMT_18 :
			return (color_t)((( LCD320240_color_18_t*)base) + numPixels );
			break;

		case LCD320240_PXLFMT_16 :
			return (color_t)((( LCD320240_color_16_t*)base) + numPixels );
			break;

		default :
			return base;
	}
}

void LCD320240_4WSPI::hwpixel(hd_hw_extent_t x0, hd_hw_extent_t y0, color_t data, hd_colors_t colorCycleLength, hd_colors_t startColorOffset)
{
	if(data == NULL){ return; }

	startColorOffset = getNewColorOffset(colorCycleLength, startColorOffset, 0);	// This line is needed to condition the user's input start color offset
	color_t value = getOffsetColor(data, startColorOffset);

	setColumnAddress( (uint16_t)x0, (uint16_t)x0);
	setRowAddress( (uint16_t)y0, (uint16_t)y0);
	uint8_t len = getBytesPerPixel( );

	writeToRAM( (uint8_t*)value, len );
}

void LCD320240_4WSPI::swpixel( hd_extent_t x0, hd_extent_t y0, color_t data, hd_colors_t colorCycleLength, hd_colors_t startColorOffset)
{
	if(data == NULL){ return; }
	if(colorCycleLength == 0){ return; }

	startColorOffset = getNewColorOffset(colorCycleLength, startColorOffset, 0);	// This line is needed to condition the user's input start color offset because it could be greater than the cycle length
	color_t value = getOffsetColor(data, startColorOffset);

	hd_hw_extent_t x0w = (hd_hw_extent_t)x0;	// Cast to hw extent type to be sure of integer values
	hd_hw_extent_t y0w = (hd_hw_extent_t)y0;

	hd_pixels_t pixOffst = wToPix(pCurrentWindow, x0, y0);			// It was already ensured that this will be in range 
	color_t dest = getOffsetColor(pCurrentWindow->data, pixOffst);	// Rely on the user's definition of a pixel's width in memory
	uint32_t len = (uint32_t)getOffsetColor(0x00, 1);				// Getting the offset from zero for one pixel tells us how many bytes to copy

	memcpy((void*)dest, (void*)value, (size_t)len);		// Copy data into the window's buffer
}

////////////////////////////////////////////////////////////
//				Display Interface Functions				  //
////////////////////////////////////////////////////////////
LCD320240_STAT_t LCD320240_4WSPI::writePacket(LCD320240_CMD_t* pcmd, uint8_t* pdata, uint16_t dlen)
{
	selectDriver();
	_spi->beginTransaction(_spisettings);

	if(pcmd != NULL)
	{
		digitalWrite(_dc, LOW);
		_spi->transfer(*(pcmd));
	}

	if( (pdata != NULL) && (dlen != 0) )
	{
		digitalWrite(_dc, HIGH);
		#if defined(__IMXRT1062__)
			transferSPIbuffer(pdata, dlen, false);
		#else
			transferSPIbuffer(pdata, dlen, ARDUINO_STILL_BROKEN );
		#endif
	}		

	_spi->endTransaction();	
	deselectDriver();
	return LCD320240_STAT_Nominal;
}

LCD320240_STAT_t LCD320240_4WSPI::transferSPIbuffer(uint8_t* pdata, size_t count, bool arduinoStillBroken ){
	// For Teensy 4.x, we can use the faster DMA-enabled transfer
	#if defined(__IMXRT1062__)
		// The Teensy 4.x SPI library has optimized DMA transfers for large blocks
		_spi->transfer(pdata, count);
		return LCD320240_STAT_Nominal;
	#else
		// For other platforms, fall back to byte-by-byte
		if(arduinoStillBroken){
			for(size_t indi = 0; indi < count; indi++){
				_spi->transfer((uint8_t)*(pdata + indi));
			}
			return LCD320240_STAT_Nominal;
		}
		else{
			_spi->transfer(pdata, count);
			return LCD320240_STAT_Nominal;
		}
	#endif
}

LCD320240_STAT_t LCD320240_4WSPI::selectDriver( void )
{
	digitalWrite(_cs, LOW);
	return LCD320240_STAT_Nominal;
}

LCD320240_STAT_t LCD320240_4WSPI::deselectDriver( void )
{
	digitalWrite(_cs, HIGH);
	return LCD320240_STAT_Nominal;
}

LCD320240_STAT_t LCD320240_4WSPI::setSPIFreq( uint32_t freq )
{
	SPISettings tempSettings(freq, LCD320240_SPI_DATA_ORDER, LCD320240_SPI_MODE);
	_spisettings = tempSettings;
	return LCD320240_STAT_Nominal;
}

////////////////////////////////////////////////////////////
//				Basic Control Functions					  //
////////////////////////////////////////////////////////////
LCD320240_STAT_t LCD320240_4WSPI::swReset( void )
{
 	LCD320240_STAT_t retval = 	LCD320240_STAT_Nominal;

 	LCD320240_CMD_t cmd = LCD320240_CMD_SWRST;
	retval = writePacket(&cmd);
	return retval;
}

LCD320240_STAT_t LCD320240_4WSPI::sleepIn( void )
{
	LCD320240_STAT_t retval = LCD320240_STAT_Nominal;
	LCD320240_CMD_t cmd = LCD320240_CMD_SLPIN;
	retval = writePacket(&cmd);
	return retval;
}

LCD320240_STAT_t LCD320240_4WSPI::sleepOut( void )
{
	LCD320240_STAT_t retval = LCD320240_STAT_Nominal;

	LCD320240_CMD_t cmd = LCD320240_CMD_SLPOUT;
	retval = writePacket(&cmd );
	return retval;
}

LCD320240_STAT_t LCD320240_4WSPI::partialModeOn( void )
{
	LCD320240_STAT_t retval = LCD320240_STAT_Nominal;

	LCD320240_CMD_t cmd = LCD320240_CMD_PTLON;
	retval = writePacket(&cmd);
	return retval;
}

LCD320240_STAT_t LCD320240_4WSPI::normalDisplayModeOn( void )
{
	LCD320240_STAT_t retval = LCD320240_STAT_Nominal;

	LCD320240_CMD_t cmd = LCD320240_CMD_PTLON;
	retval = writePacket(&cmd);
	return retval;
}

LCD320240_STAT_t LCD320240_4WSPI::setInversion( bool on )
{
	LCD320240_STAT_t retval = LCD320240_STAT_Nominal;

	LCD320240_CMD_t cmd = LCD320240_CMD_INVOFF;
	if( on )
	{
		cmd = LCD320240_CMD_INVON;
	}
	retval = writePacket(&cmd);
	return retval;
}

LCD320240_STAT_t LCD320240_4WSPI::setPower( bool on )
{
	LCD320240_STAT_t retval = LCD320240_STAT_Nominal;

	LCD320240_CMD_t cmd = LCD320240_CMD_OFF;
	if( on )
	{
		cmd = LCD320240_CMD_ON;
	}
	retval = writePacket(&cmd);
	return retval;
}

LCD320240_STAT_t LCD320240_4WSPI::setColumnAddress( uint16_t start, uint16_t end )
{
	LCD320240_STAT_t retval = LCD320240_STAT_Nominal;

	LCD320240_CMD_t cmd = LCD320240_CMD_CASET;
	uint8_t buff[4] = {(start >> 8), (start & 0x00FF), (end >> 8), (end & 0x00FF)};
	retval = writePacket(&cmd, buff, 4);
	return retval;
}

LCD320240_STAT_t LCD320240_4WSPI::setRowAddress( uint16_t start, uint16_t end )
{
	LCD320240_STAT_t retval = LCD320240_STAT_Nominal;

	LCD320240_CMD_t cmd = LCD320240_CMD_RASET;
	uint8_t buff[4] = {(start >> 8), (start & 0x00FF), (end >> 8), (end & 0x00FF)};
	retval = writePacket(&cmd, buff, 4);
	return retval;
}

LCD320240_STAT_t LCD320240_4WSPI::writeToRAM( uint8_t* pdata, uint16_t numBytes )
{
	LCD320240_STAT_t retval = LCD320240_STAT_Nominal;

	LCD320240_CMD_t cmd = LCD320240_CMD_WRRAM;
	retval = writePacket(&cmd, pdata, numBytes);
	return retval;
}

////////////////////////////////////////////////////////////
//			Functions to configure the display			  //
////////////////////////////////////////////////////////////
LCD320240_STAT_t LCD320240_4WSPI::setMemoryAccessControl( bool mx, bool my, bool mv, bool ml, bool bgr, bool mh )
{
	LCD320240_STAT_t retval = LCD320240_STAT_Nominal;

	LCD320240_CMD_t cmd = LCD320240_CMD_WRMADCTL;
	uint8_t buff = 0x00;
	if( my ){ buff |= 0x80; }
	if( mx ){ buff |= 0x40; }
	if( mv ){ buff |= 0x20; }
	if( ml ){ buff |= 0x10; }
	if( bgr ){ buff |= 0x08; }
	if( mh ){ buff |= 0x04; }
	retval = writePacket(&cmd, &buff, 1);
	return retval;
}

LCD320240_STAT_t LCD320240_4WSPI::selectGammaCurve( uint8_t bmNumber )
{
	LCD320240_STAT_t retval = LCD320240_STAT_Nominal;

	LCD320240_CMD_t cmd = LCD320240_CMD_GAMST;
	uint8_t buff = bmNumber;
	retval = writePacket(&cmd, &buff, 1);
	return retval;
}

LCD320240_STAT_t LCD320240_4WSPI::setPartialArea(uint16_t start, uint16_t end )
{
	LCD320240_STAT_t retval = LCD320240_STAT_Nominal;

	LCD320240_CMD_t cmd = LCD320240_CMD_PTLAREA;
	uint8_t buff[4] = {(start >> 8), (start & 0x00FF), (end >> 8), (end & 0x00FF)};
	retval = writePacket(&cmd, buff, 4);
	return retval;
}

LCD320240_STAT_t LCD320240_4WSPI::setVerticalScrolling( uint16_t tfa, uint16_t vsa, uint16_t bfa )
{
	LCD320240_STAT_t retval = LCD320240_STAT_Nominal;

	LCD320240_CMD_t cmd = LCD320240_CMD_WRVSCRL;
	uint8_t buff[6] = {(tfa >> 8), (tfa & 0x00FF), (vsa >> 8), (vsa & 0x00FF), (bfa >> 8), (bfa & 0x00FF)};
	retval = writePacket(&cmd, buff, 6);
	return retval;
}

LCD320240_STAT_t LCD320240_4WSPI::setVerticalScrollingStartAddress( uint16_t ssa )
{
	LCD320240_STAT_t retval = LCD320240_STAT_Nominal;

	LCD320240_CMD_t cmd = LCD320240_CMD_WRVSSA;
	uint8_t buff[2] = {(ssa >> 8), (ssa & 0x00FF)};
	retval = writePacket(&cmd, buff, 2);
	return retval;
}

LCD320240_STAT_t LCD320240_4WSPI::setIdleMode( bool on )
{
	LCD320240_STAT_t retval = LCD320240_STAT_Nominal;

	LCD320240_CMD_t cmd = LCD320240_CMD_IDLOFF;
	if( on )
	{
		cmd = LCD320240_CMD_IDLON;
	}
	retval = writePacket(&cmd);
	return retval;
}

LCD320240_STAT_t LCD320240_4WSPI::setInterfacePixelFormat( uint8_t CTRLintfc )
{
	LCD320240_STAT_t retval = LCD320240_STAT_Nominal;

	LCD320240_CMD_t cmd = LCD320240_CMD_WRPXFMT;
	uint8_t buff = (CTRLintfc & 0x07);

	if( buff == LCD320240_PXLFMT_16 ){ _pxlfmt = LCD320240_PXLFMT_16; }
	if( buff == LCD320240_PXLFMT_18 ){ _pxlfmt = LCD320240_PXLFMT_18; }

	retval = writePacket(&cmd, &buff, 1);
	return retval;
}

LCD320240_STAT_t LCD320240_4WSPI::setTearingEffectLine( bool on )
{
	LCD320240_STAT_t retval = LCD320240_STAT_Nominal;

	LCD320240_CMD_t cmd = LCD320240_CMD_TELOFF;
	if( on )
	{
		cmd = LCD320240_CMD_TELON;
	}
	retval = writePacket(&cmd);
	return retval;
}

LCD320240_STAT_t LCD320240_4WSPI::setNormalFramerate( uint8_t diva, uint8_t vpa )
{
	LCD320240_STAT_t retval = LCD320240_STAT_Nominal;

	LCD320240_CMD_t cmd = LCD320240_CMD_WRNMLFRCTL;
	uint8_t buff[2] = {diva, vpa};
	retval = writePacket(&cmd, buff, 2);
	return retval;
}

LCD320240_STAT_t LCD320240_4WSPI::setIdleFramerate( uint8_t divb, uint8_t vpb )
{
	LCD320240_STAT_t retval = LCD320240_STAT_Nominal;

	LCD320240_CMD_t cmd = LCD320240_CMD_WRIDLFRCTL;
	uint8_t buff[2] = {divb, vpb};
	retval = writePacket(&cmd, buff, 2);
	return retval;
}

LCD320240_STAT_t LCD320240_4WSPI::setPartialFramerate( uint8_t divc, uint8_t vpc )
{
	LCD320240_STAT_t retval = LCD320240_STAT_Nominal;

	LCD320240_CMD_t cmd = LCD320240_CMD_WRPTLFRCTL;
	uint8_t buff[2] = {divc, vpc};
	retval = writePacket(&cmd, buff, 2);
	return retval;
}

LCD320240_STAT_t LCD320240_4WSPI::setPowerControl1( uint8_t vrh, uint8_t vc )
{
	LCD320240_STAT_t retval = LCD320240_STAT_Nominal;

	LCD320240_CMD_t cmd = LCD320240_CMD_WRPWCTL1;
	uint8_t buff[2] = {(vrh & 0x1F), (vc & 0x07)};
	retval = writePacket(&cmd, buff, 2);
	return retval;
}

LCD320240_STAT_t LCD320240_4WSPI::setPowerControl2( uint8_t bt )
{
	LCD320240_STAT_t retval = LCD320240_STAT_Nominal;

	LCD320240_CMD_t cmd = LCD320240_CMD_WRPWCTL2;
	uint8_t buff = (bt & 0x07);
	retval = writePacket(&cmd, &buff, 1);
	return retval;
}

LCD320240_STAT_t LCD320240_4WSPI::setPowerControl3( uint8_t apa )
{
	LCD320240_STAT_t retval = LCD320240_STAT_Nominal;

	LCD320240_CMD_t cmd = LCD320240_CMD_WRPWCTL3;
	uint8_t buff = (apa & 0x07);
	retval = writePacket(&cmd, &buff, 1);
	return retval;
}

LCD320240_STAT_t LCD320240_4WSPI::setPowerControl4( uint8_t apb )
{
	LCD320240_STAT_t retval = LCD320240_STAT_Nominal;

	LCD320240_CMD_t cmd = LCD320240_CMD_WRPWCTL4;
	uint8_t buff = (apb & 0x07);
	retval = writePacket(&cmd, &buff, 1);
	return retval;
}

LCD320240_STAT_t LCD320240_4WSPI::setPowerControl5( uint8_t apc )
{
	LCD320240_STAT_t retval = LCD320240_STAT_Nominal;

	LCD320240_CMD_t cmd = LCD320240_CMD_WRPWCTL5;
	uint8_t buff = (apc & 0x07);
	retval = writePacket(&cmd, &buff, 1);
	return retval;
}

LCD320240_STAT_t LCD320240_4WSPI::setVCOMControl1( uint8_t vmh, uint8_t vml )
{
	LCD320240_STAT_t retval = LCD320240_STAT_Nominal;

	LCD320240_CMD_t cmd = LCD320240_CMD_WRVCOMCTL1;
	uint8_t buff[2] = {(vmh & 0x7F), (vml & 0x7F)};
	retval = writePacket(&cmd, buff, 2);
	return retval;
}

LCD320240_STAT_t LCD320240_4WSPI::setVCOMOffsetControl( bool nVM, uint8_t vmf )
{
	LCD320240_STAT_t retval = LCD320240_STAT_Nominal;

	LCD320240_CMD_t cmd = LCD320240_CMD_WRVCMOFSTCTL;
	uint8_t buff = (vmf & 0x7F);
	if( nVM )
	{
		buff |= 0x80;
	}
	retval = writePacket(&cmd, &buff, 1);
	return retval;
}

LCD320240_STAT_t LCD320240_4WSPI::setSrcDriverDir( bool crl )
{
	LCD320240_STAT_t retval = LCD320240_STAT_Nominal;

	LCD320240_CMD_t cmd = LCD320240_CMD_WRSDRVDIR;
	uint8_t buff = 0x00;
	if( crl )
	{
		buff |= 0x01;
	}
	retval = writePacket(&cmd, &buff, 1);
	return retval;
}

LCD320240_STAT_t LCD320240_4WSPI::setGateDriverDir( bool ctb )
{
	LCD320240_STAT_t retval = LCD320240_STAT_Nominal;

	LCD320240_CMD_t cmd = LCD320240_CMD_WRGDRVDIR;
	uint8_t buff = 0x00;
	if( ctb )
	{
		buff |= 0x01;
	}
	retval = writePacket(&cmd, &buff, 1);
	return retval;
}

LCD320240_STAT_t LCD320240_4WSPI::setGamRSel( bool gamrsel )
{
	LCD320240_STAT_t retval = LCD320240_STAT_Nominal;

	LCD320240_CMD_t cmd = LCD320240_CMD_WRGAMRS;
	uint8_t buff = 0x00;
	if( gamrsel )
	{
		buff |= 0x01;
	}
	retval = writePacket(&cmd, &buff, 1);
	return retval;
}

LCD320240_STAT_t LCD320240_4WSPI::setPositiveGamCorr( uint8_t* gam16byte )
{ 
	LCD320240_STAT_t retval = LCD320240_STAT_Nominal;

	LCD320240_CMD_t cmd = LCD320240_CMD_WRPGCS;
	retval = writePacket(&cmd, gam16byte, 16);
	return retval;
}

LCD320240_STAT_t LCD320240_4WSPI::setNegativeGamCorr( uint8_t* gam16byte )
{ 
	LCD320240_STAT_t retval = LCD320240_STAT_Nominal;

	LCD320240_CMD_t cmd = LCD320240_CMD_WRNGCS;
	retval = writePacket(&cmd, gam16byte, 16);
	return retval;
}

////////////////////////////////////////////////////////////
//				Optimized Drawing Functions				  //
////////////////////////////////////////////////////////////
void LCD320240_4WSPI::hwxline(hd_hw_extent_t x0, hd_hw_extent_t y0, hd_hw_extent_t len, color_t data, hd_colors_t colorCycleLength, hd_colors_t startColorOffset, bool goLeft)
{
	if(data == NULL){ return; }
	if( len < 1 ){ return; }

	startColorOffset = getNewColorOffset(colorCycleLength, startColorOffset, 0);	// This line is needed to condition the user's input start color offset

	color_t value = getOffsetColor(data, startColorOffset);
	uint8_t bpp = getBytesPerPixel( );

	if( goLeft )
	{ 
		setMemoryAccessControl( true, true, false, false, true, false ); 
		x0 = (xExt - 1) - x0;
	}
	hd_hw_extent_t x1 = x0 + (len - 1);

	// Setup the valid area to draw...
	setColumnAddress( x0, x1);
	setRowAddress(y0, y0);

	LCD320240_CMD_t cmd = LCD320240_CMD_WRRAM;
	writePacket(&cmd);					// Send the command to enable writing to RAM but don't send any data yet

	// Now, we need to send data with as little overhead as possible, while respecting the start offset and color cycle length and everything else...
	selectDriver();
	digitalWrite(_dc, HIGH);
	_spi->beginTransaction(_spisettings);

	if(colorCycleLength == 1)
	{
		// Special case that can be handled with a lot less thinking (so faster)
		uint8_t speedupArry[LCD320240_MAX_X*LCD320240_MAX_BPP];
		for(uint16_t indi = 0; indi < len; indi++)
		{
			for(uint8_t indj = 0; indj < bpp; indj++)
			{
				speedupArry[ indj + (indi*bpp) ] = *((uint8_t*)(data) + indj);
			}
		}
		#if defined(__IMXRT1062__)
			transferSPIbuffer(speedupArry, len*bpp, false);
		#else
			transferSPIbuffer(speedupArry, len*bpp, ARDUINO_STILL_BROKEN );
		#endif
	}
	else
	{
		uint16_t pixelsToDraw = 0;
		uint16_t pixelsDrawn = 0;

		while(len != 0)
		{
			// Let's figure out how many pixels we can draw right now contiguously.. (thats from the start offset to the full legnth of the cycle)
			uint16_t pixelsAvailable = colorCycleLength - startColorOffset;
			value = getOffsetColor(data, startColorOffset);
			
			if( pixelsAvailable >= len ){ pixelsToDraw = len; }
			else if( pixelsAvailable < len ){ pixelsToDraw = pixelsAvailable; }

			#if defined(__IMXRT1062__)
				transferSPIbuffer((uint8_t*)value, bpp*pixelsToDraw, false);
			#else
				transferSPIbuffer((uint8_t*)value, bpp*pixelsToDraw, ARDUINO_STILL_BROKEN );
			#endif

			len -= pixelsToDraw;
			pixelsDrawn = pixelsToDraw;
			startColorOffset = getNewColorOffset(colorCycleLength, startColorOffset, pixelsDrawn);
		}
	}

	_spi->endTransaction();	
	deselectDriver();

	if( goLeft ){ setMemoryAccessControl( false, true, false, false, true, false ); } // Reset to defaults
}

void LCD320240_4WSPI::hwyline(hd_hw_extent_t x0, hd_hw_extent_t y0, hd_hw_extent_t len, color_t data, hd_colors_t colorCycleLength, hd_colors_t startColorOffset, bool goUp)
{
	if(data == NULL){ return; } 
	if( len < 1 ){ return; }

	startColorOffset = getNewColorOffset(colorCycleLength, startColorOffset, 0);	// This line is needed to condition the user's input start color offset
	color_t value = getOffsetColor(data, startColorOffset);
	uint8_t bpp = getBytesPerPixel( );

	if( goUp )
	{ 
		setMemoryAccessControl( false, false, false, false, true, false ); 
		y0 = (yExt - 1) - y0; 
	}
	hd_hw_extent_t y1 = y0 + (len - 1);
	
	// Setup the valid area to draw...
	setColumnAddress( x0, x0);
	setRowAddress(y0, y1);

	LCD320240_CMD_t cmd = LCD320240_CMD_WRRAM;
	writePacket(&cmd);					// Send the command to enable writing to RAM but don't send any data yet

	// Now, we need to send data with as little overhead as possible, while respecting the start offset and color cycle length and everything else...
	selectDriver();
	digitalWrite(_dc, HIGH);
	_spi->beginTransaction(_spisettings);


	if(colorCycleLength == 1)
	{
		// Special case that can be handled with a lot less thinking (so faster)
		uint8_t speedupArry[LCD320240_MAX_Y*LCD320240_MAX_BPP];
		for(uint16_t indi = 0; indi < len; indi++)
		{
			for(uint8_t indj = 0; indj < bpp; indj++)
			{
				speedupArry[ indj + (indi*bpp) ] = *((uint8_t*)(data) + indj);
			}
		}
		#if defined(__IMXRT1062__)
			transferSPIbuffer(speedupArry, len*bpp, false);
		#else
			transferSPIbuffer(speedupArry, len*bpp, ARDUINO_STILL_BROKEN );
		#endif
	}
	else
	{
		uint16_t pixelsToDraw = 0;
		uint16_t pixelsDrawn = 0;

		while(len != 0)
		{
			// Let's figure out how many pixels we can draw right now contiguously.. (thats from the start offset to the full legnth of the cycle)
			uint16_t pixelsAvailable = colorCycleLength - startColorOffset;
			value = getOffsetColor(data, startColorOffset);
			
			if( pixelsAvailable >= len ){ pixelsToDraw = len; }
			else if( pixelsAvailable < len ){ pixelsToDraw = pixelsAvailable; }

			#if defined(__IMXRT1062__)
				transferSPIbuffer((uint8_t*)value, bpp*pixelsToDraw, false);
			#else
				transferSPIbuffer((uint8_t*)value, bpp*pixelsToDraw, ARDUINO_STILL_BROKEN );
			#endif

			len -= pixelsToDraw;
			pixelsDrawn = pixelsToDraw;
			startColorOffset = getNewColorOffset(colorCycleLength, startColorOffset, pixelsDrawn);
		}
	}

	_spi->endTransaction();	
	deselectDriver();

	if( goUp )
	{ 
		setMemoryAccessControl( false, true, false, false, true, false );
	}
}

void LCD320240_4WSPI::hwfillFromArray(hd_hw_extent_t x0, hd_hw_extent_t y0, hd_hw_extent_t x1, hd_hw_extent_t y1, color_t data, hd_pixels_t numPixels, bool Vh)
{
	if(numPixels == 0){ return; }
	if(data == NULL ){ return; }

	uint8_t bpp = getBytesPerPixel();

	if( Vh )
	{ 
		setMemoryAccessControl( true, true, true, false, true, false );

		setColumnAddress( y0, y1);
		setRowAddress(x0, x1);
	}
	else
	{
		setColumnAddress( x0, x1);
		setRowAddress(y0, y1);
	}
	

	LCD320240_CMD_t cmd = LCD320240_CMD_WRRAM;
	writePacket(&cmd);					// Send the command to enable writing to RAM but don't send any data yet

	selectDriver();
	digitalWrite(_dc, HIGH);
	_spi->beginTransaction(_spisettings);

	// For Teensy 4.x, we can use the optimized transfer
	#if defined(__IMXRT1062__)
		// Check if data is already in the correct byte order (big-endian)
		// If the VideoPlayer already swapped bytes, we can send directly
		_spi->transfer((uint8_t*)data, bpp*numPixels);
	#else
		transferSPIbuffer((uint8_t*)data, bpp*numPixels, ARDUINO_STILL_BROKEN);
	#endif

	_spi->endTransaction();	
	deselectDriver();

	if( Vh ){ setMemoryAccessControl( true, true, false, false, true, false ); }
}

////////////////////////////////////////////////////////////
//			Display-specific Implementation				  //
////////////////////////////////////////////////////////////
LCD320240_STAT_t LCD320240_4WSPI::begin(uint8_t dcPin, uint8_t csPin, uint8_t blPin, SPIClass &spiInterface, uint32_t spiFreq)
{
	// Associate pins
	_dc = dcPin;
	_rst = 0xFF; // Not using reset pin
	_cs = csPin;
	_bl = blPin;
	_spi = &spiInterface;

	if(spiFreq != LCD320240_SPI_DEFAULT_FREQ)
	{
		setSPIFreq( spiFreq );
	}

	_spi->begin();

	// Set pinmodes
	pinMode(_cs, OUTPUT);
	pinMode(_dc, OUTPUT);
	pinMode(_bl, OUTPUT);

	// Set pins to default positions
	digitalWrite(_cs, HIGH);
	digitalWrite(_dc, HIGH);
	digitalWrite(_bl, LOW);

	// Setup the default window
	setWindowDefaults(pCurrentWindow);

	// Power up the device

	// try starting SPI with a simple byte transmisssion to 'set' the SPI peripherals
	uint8_t temp_buff[1];
	_spi->beginTransaction(_spisettings);
	_spi->transfer(temp_buff, 1);
	_spi->endTransaction();

	startup();	
	defaultConfigure();

	return LCD320240_STAT_Nominal;
}

LCD320240_STAT_t LCD320240_4WSPI::defaultConfigure( void )
{
	LCD320240_STAT_t retval = LCD320240_STAT_Nominal;

	retval = sleepOut();
	if(retval != LCD320240_STAT_Nominal){ return retval; }

  	delay(20);

  	retval = selectGammaCurve( 0x01 );
	if(retval != LCD320240_STAT_Nominal){ return retval; }

	retval = setNormalFramerate( 0x00, 0x1B );
	if(retval != LCD320240_STAT_Nominal){ return retval; }

	retval = setPowerControl1( 0x21, 0x00 );
 	if(retval != LCD320240_STAT_Nominal){ return retval; }

  	retval = setPowerControl2( 0x10 );
  	if(retval != LCD320240_STAT_Nominal){ return retval; }

  	retval = setVCOMControl1( 0x3E, 0x28 );
	if(retval != LCD320240_STAT_Nominal){ return retval; }

	retval = setVCOMOffsetControl( true, 0x86 );
	if(retval != LCD320240_STAT_Nominal){ return retval; }
	
	retval = setInversion( true );
	if(retval != LCD320240_STAT_Nominal){ return retval; }
	
	retval = setInterfacePixelFormat( 0x05 );
	if(retval != LCD320240_STAT_Nominal){ return retval; }

	retval = setColumnAddress( LCD320240_START_COL, LCD320240_STOP_COL );
	if(retval != LCD320240_STAT_Nominal){ return retval; }

	retval = setRowAddress( LCD320240_START_ROW, LCD320240_STOP_ROW );
	if(retval != LCD320240_STAT_Nominal){ return retval; }

	retval = setMemoryAccessControl( false, true, false, false, true, false );
	if(retval != LCD320240_STAT_Nominal){ return retval; }

	uint8_t pgam[16] =  { 0x0F, 0x16, 0x14, 0x0A, 0x0D, 0x06, 0x43, 0x75, 0x33, 0x06, 0x0E, 0x00, 0x0C, 0x09, 0x08 };
	retval = setPositiveGamCorr( pgam );
	if(retval != LCD320240_STAT_Nominal){ return retval; }

	uint8_t ngam[16] = { 0x08, 0x2B, 0x2D, 0x04, 0x10, 0x04, 0x3E, 0x24, 0x4E, 0x04, 0x0F, 0x0E, 0x35, 0x38, 0x0F };
	setNegativeGamCorr( ngam );
	if(retval != LCD320240_STAT_Nominal){ return retval; }

  	delay(20);

  	setPower( true );

  	return LCD320240_STAT_Nominal;
}

void LCD320240_4WSPI::startup( void )
{
	// Assume that VDD and VCC are stable when this function is called
	if(_rst != 0xFF){ digitalWrite( _rst , HIGH); }
  	delay(10);
  	if(_rst != 0xFF){ digitalWrite( _rst , LOW); }
  	delay(10);
  	if(_rst != 0xFF){ digitalWrite( _rst , HIGH); }
  	delay(120);
	// Now you can do initialization
}

void LCD320240_4WSPI::clearDisplay( void )
{
	// Store the old window pointer: 
	wind_info_t * ptempWind = pCurrentWindow;

	// Make a new default window
	wind_info_t window;
	pCurrentWindow = &window;

	// Ensure the window is set up so that you can clear the whole screen
	setWindowDefaults(&window);

	// Make a local 'black' color 
	LCD320240_color_18_t black;
	black.r = 0;
	black.g = 0;
	black.b = 0;

	// Fill the temporary window with black
	fillWindow((color_t)&black);												// Pass the address of the buffer b/c we know it will be safe no matter what SSD1357 color mode is used

	// Restore the previous window
	pCurrentWindow = ptempWind;
}

void LCD320240_4WSPI::setWindowDefaults(wind_info_t * pwindow)
{
	// Fills out the default window structure with more or less reasonable defaults
	pwindow->xMin = LCD320240_START_COL;
	pwindow->yMin = LCD320240_START_ROW;
	pwindow->xMax = LCD320240_STOP_COL;
	pwindow->yMax = LCD320240_STOP_ROW;
	pwindow->cursorX = 0;							// cursor values are in window coordinates
	pwindow->cursorY = 0;
	pwindow->xReset = 0;
	pwindow->yReset = 0;
	
	pwindow->lastCharacter.data = NULL;
	pwindow->lastCharacter.xLoc = NULL;
	pwindow->lastCharacter.yLoc = NULL;
	pwindow->lastCharacter.xDim = 0;
	pwindow->lastCharacter.yDim = 0;
	pwindow->lastCharacter.numPixels = 0;
	pwindow->lastCharacter.show = false;
	pwindow->lastCharacter.causesNewline = false;
	
	pwindow->bufferMode = false;			// Start out in direct mode
	pwindow->data = NULL;				// No window data yet
	pwindow->numPixels = 0;
	pwindow->dynamic = false;
	setWindowColorSequence(pwindow, NULL, 1, 0);	// Setup the default color (Which is NULL, so that you know it is not set yet)
}

void LCD320240_4WSPI::setBacklight(uint8_t b){
#if defined(ARDUINO_ARCH_ESP32)
	ledcAttachPin(_bl,15);
	ledcSetup(15,12000,8);
	ledcWrite(15, 255-b);
#else
	analogWrite(_bl, 255-b);
#endif
}