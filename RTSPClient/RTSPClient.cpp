// RTSPClient.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

//////////////////////////////////////////////////////////////////////////
//Appendix A
//
//   The following code can be used to create a quantization table from a
//   Q factor:

/*
 * Table K.1 from JPEG spec.
 */

unsigned char Qtbl[128] = 
{
	0x35, 0x25, 0x28, 0x2f, 0x28, 0x21, 0x35, 0x2f,
	0x2b, 0x2f, 0x3c, 0x39, 0x35, 0x3f, 0x50, 0x85, 
	0x57, 0x50, 0x49, 0x49, 0x50, 0xa3, 0x75, 0x7b, 
	0x61, 0x85, 0xc1, 0xaa, 0xcb, 0xc8, 0xbe, 0xaa, 
	0xbb, 0xb7, 0xd5, 0xf0, 0xff, 0xff, 0xd5, 0xe2, 
	0xff, 0xe6, 0xb7, 0xbb, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xcf, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0x39, 0x3c, 0x3c, 0x50, 0x46, 0x50, 0x9d, 0x57, 
	0x57, 0x9d, 0xff, 0xdc, 0xbb, 0xdc, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

static const int jpeg_luma_quantizer[64] = {
        16, 11, 10, 16, 24, 40, 51, 61,
        12, 12, 14, 19, 26, 58, 60, 55,
        14, 13, 16, 24, 40, 57, 69, 56,
        14, 17, 22, 29, 51, 87, 80, 62,
        18, 22, 37, 56, 68, 109, 103, 77,
        24, 35, 55, 64, 81, 104, 113, 92,
        49, 64, 78, 87, 103, 121, 120, 101,
        72, 92, 95, 98, 112, 100, 103, 99
};

/*
 * Table K.2 from JPEG spec.
 */
static const int jpeg_chroma_quantizer[64] = {
        17, 18, 24, 47, 99, 99, 99, 99,
        18, 21, 26, 66, 99, 99, 99, 99,
        24, 26, 56, 99, 99, 99, 99, 99,
        47, 66, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99
};

/*
 * Call MakeTables with the Q factor and two unsigned char[64] return arrays
 */
void MakeTables(int q, unsigned char *lqt, unsigned char *cqt)
{
  int i;
  int factor = q;

  if (q < 1) factor = 1;
  if (q > 99) factor = 99;
  if (q < 50)
    q = 5000 / factor;
  else
    q = 200 - factor*2;

  for (i=0; i < 64; i++) {
    int lq = (jpeg_luma_quantizer[i] * q + 50) / 100;
    int cq = (jpeg_chroma_quantizer[i] * q + 50) / 100;

    /* Limit the quantizers to 1 <= q <= 255 */
    if (lq < 1) lq = 1;
    else if (lq > 255) lq = 255;
    lqt[i] = lq;

    if (cq < 1) cq = 1;
    else if (cq > 255) cq = 255;
    cqt[i] = cq;
  }
}


//////////////////////////////////////////////////////////////////////////

//Appendix B
//
//   The following routines can be used to create the JPEG marker segments
//   corresponding to the table-specification data that is absent from the
//   RTP/JPEG body.

unsigned char lum_dc_codelens[] = {
        0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
};

unsigned char lum_dc_symbols[] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
};

unsigned char lum_ac_codelens[] = {
        0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d,
};

unsigned char lum_ac_symbols[] = {
        0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
        0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
        0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
        0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
        0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
        0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
        0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
        0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
        0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
        0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
        0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
        0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
        0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
        0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
        0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
        0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
        0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
        0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
        0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
        0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
        0xf9, 0xfa,
};

unsigned char chm_dc_codelens[] = {
        0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
};

unsigned char chm_dc_symbols[] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
};

unsigned char chm_ac_codelens[] = {
        0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77,
};

unsigned char chm_ac_symbols[] = {
        0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
        0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
        0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
        0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
        0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
        0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
        0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
        0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
        0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
        0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
        0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
        0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
        0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
        0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
        0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
        0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
        0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
        0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
        0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
        0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
        0xf9, 0xfa,
};

unsigned char *
MakeQuantHeader(unsigned char *p, unsigned char *qt, int tableNo)
{
        *p++ = 0xff;
        *p++ = 0xdb;            /* DQT */
        *p++ = 0;               /* length msb */
        *p++ = 67;              /* length lsb */
        *p++ = tableNo;
        memcpy(p, qt, 64);
        return (p + 64);
}

unsigned char *
MakeHuffmanHeader(unsigned char *p, unsigned char *codelens, int ncodes,
                  unsigned char *symbols, int nsymbols, int tableNo,
                  int tableClass)
{
        *p++ = 0xff;
        *p++ = 0xc4;            /* DHT */
        *p++ = 0;               /* length msb */
        *p++ = 3 + ncodes + nsymbols; /* length lsb */
        *p++ = (tableClass << 4) | tableNo;
        memcpy(p, codelens, ncodes);
        p += ncodes;
        memcpy(p, symbols, nsymbols);
        p += nsymbols;
        return (p);
}

unsigned char *
MakeDRIHeader(unsigned char *p, unsigned short dri) {
        *p++ = 0xff;
        *p++ = 0xdd;				/* DRI */
        *p++ = 0x00;				/* length msb */
        *p++ = 0x04;              /* length lsb */
        *p++ = 0x00;				/* dri msb */
        *p++ = 0x54;				/* dri lsb */
        return (p);
}

/*
 *  Arguments:
 *    type, width, height: as supplied in RTP/JPEG header
 *    lqt, cqt: quantization tables as either derived from
 *         the Q field using MakeTables() or as specified
 *         in section 4.2.
 *    dri: restart interval in MCUs, or 0 if no restarts.
 *
 *    p: pointer to return area
 *
 *  Return value:
 *    The length of the generated headers.
 *
 *    Generate a frame and scan headers that can be prepended to the
 *    RTP/JPEG data payload to produce a JPEG compressed image in
 *    interchange format (except for possible trailing garbage and
 *    absence of an EOI marker to terminate the scan).
 */

unsigned char* SOI(unsigned char *p)
{
	*p++ = 0xff;
	*p++ = 0xd8;            /* SOI */

	return p;
}

unsigned char* APP0(unsigned char *p)
{
	*p++ = 0xff;
	*p++ = 0xe0;
	*p++ = 0x00;
	*p++ = 0x10;
	*p++ = 0x4a;
	*p++ = 0x46;
	*p++ = 0x49;
	*p++ = 0x46;
	*p++ = 0x00;
	*p++ = 0x01;
	*p++ = 0x01;
	*p++ = 0x01;
	*p++ = 0x00;
	*p++ = 0x60;
	*p++ = 0x00;
	*p++ = 0x60;
	*p++ = 0x00;
	*p++ = 0x00;

	return p;
}

unsigned char* DQT(unsigned char *p)
{
	unsigned char lqt[64] = 
	{
		0x35, 0x25, 0x28, 0x2f, 0x28, 0x21, 0x35, 0x2f, 
		0x2b, 0x2f, 0x3c, 0x39, 0x35, 0x3f, 0x50, 0x85, 
		0x57, 0x50, 0x49, 0x49, 0x50, 0xa3, 0x75, 0x7b, 
		0x61, 0x85, 0xc1, 0xaa, 0xcb, 0xc8, 0xbe, 0xaa, 
		0xbb, 0xb7, 0xd5, 0xf0, 0xff, 0xff, 0xd5, 0xe2, 
		0xff, 0xe6, 0xb7, 0xbb, 0xff, 0xff, 0xff, 0xff, 
		0xff, 0xff, 0xff, 0xff, 0xff, 0xcf, 0xff, 0xff, 
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	};
	unsigned char cqt[64] = 
	{
		0x39, 0x3c, 0x3c, 0x50, 0x46, 0x50, 0x9d, 0x57, 
		0x57, 0x9d, 0xff, 0xdc, 0xbb, 0xdc, 0xff, 0xff, 
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	};

	p = MakeQuantHeader(p, lqt, 0);
	p = MakeQuantHeader(p, cqt, 1);
	return p;
}

unsigned char* DRI(unsigned char* p)
{
	int dri = 84;
	if (dri != 0)
		p = MakeDRIHeader(p, dri);

	return p;
}

unsigned char *SOF(unsigned char *p)
{
	*p++ = 0xff;
	*p++ = 0xc0;				/* SOF */
	*p++ = 0x00;               /* length msb */
	*p++ = 0x11;				/* length lsb */
	*p++ = 0x08;               /* 8-bit precision */
	*p++ = 0x04;				/* height msb */
	*p++ = 0x38;				/* height lsb */
	*p++ = 0x07;				/* width msb */
	*p++ = 0x80;               /* wudth lsb */

	*p++ = 0x03;               /* number of components */
	*p++ = 0x01;               /* comp 0 */
	
	//if (type == 0)
		//*p++ = 0x21;			/* hsamp = 2, vsamp = 1 */
	//else
		*p++ = 0x22;			/* hsamp = 2, vsamp = 2 */

	*p++ = 0x00;               /* quant table 0 */
	*p++ = 0x02;               /* comp 1 */
	*p++ = 0x11;				/* hsamp = 1, vsamp = 1 */
	*p++ = 0x01;               /* quant table 1 */
	*p++ = 0x03;               /* comp 2 */
	*p++ = 0x11;				/* hsamp = 1, vsamp = 1 */
	*p++ = 0x01;               /* quant table 1 */

	return p;
}

unsigned char *DHT(unsigned char *p)
{
	p = MakeHuffmanHeader(p, lum_dc_codelens,
		sizeof(lum_dc_codelens),
		lum_dc_symbols,
		sizeof(lum_dc_symbols), 0, 0);
	p = MakeHuffmanHeader(p, lum_ac_codelens,
		sizeof(lum_ac_codelens),
		lum_ac_symbols,
		sizeof(lum_ac_symbols), 0, 1);
	p = MakeHuffmanHeader(p, chm_dc_codelens,
		sizeof(chm_dc_codelens),
		chm_dc_symbols,
		sizeof(chm_dc_symbols), 1, 0);
	p = MakeHuffmanHeader(p, chm_ac_codelens,
		sizeof(chm_ac_codelens),
		chm_ac_symbols,
		sizeof(chm_ac_symbols), 1, 1);

	return p;
}

unsigned char *SOS(unsigned char *p)
{
	*p++ = 0xff;
	*p++ = 0xda;				/* SOS */
	*p++ = 0x00;               /* length msb */
	*p++ = 0x0c;				/* length lsb */
	*p++ = 0x03;               /* 3 components */
	*p++ = 0x01;               /* comp 0 */
	*p++ = 0x00;               /* huffman table 0 */
	*p++ = 0x02;               /* comp 1 */
	*p++ = 0x11;				/* huffman table 1 */
	*p++ = 0x03;               /* comp 2 */
	*p++ = 0x11;				/* huffman table 1 */
	*p++ = 0x00;               /* first DCT coeff */
	*p++ = 0x3f;				/* last DCT coeff */
	*p++ = 0x00;               /* sucessive approx. */	

	return p;
}

int MakeHeaders(unsigned char *p, int type, int w, int h, unsigned char *lqt,
                unsigned char *cqt, unsigned short dri)
{
	unsigned char *start = p;

	p = SOI(p);
	p = APP0(p);

	//below 4 sections have no sequence
	p = SOF(p);
	p = DHT(p);
	p = DRI(p);
	p = DQT(p);

	p = SOS(p);

	return (p - start);
};

void WriteHead()
{
	//MakeTables(255, lqt, cqt);

	unsigned char lqt[1];
	unsigned char cqt[1];
	unsigned char p[1024] = {0};
	int nvalue = MakeHeaders(p, 1, 1920, 1080, lqt,cqt, 84);

	FILE * pFile;
	pFile = fopen ("C:\\Users\\Hank\\Desktop\\Data.jpg","wb+");
	if (pFile!=NULL)
	{
		fwrite (p , sizeof(unsigned char), nvalue, pFile);
		fclose (pFile);
	}
}

void WriteData()
{
	char datapath[50] = {0}; 
	FILE * pReadFile = NULL;
	FILE * pWriteFile = NULL;
	unsigned char *pData = NULL;
	int nDataSize = 0, nResult = 0;

	pWriteFile = fopen ("C:\\Users\\Hank\\Desktop\\Data.jpg","ab+");
	if (pWriteFile!=NULL)
	{
		for (int nIdx = 0; nIdx < 37; nIdx++)
		{
			memset(datapath, 0x00, sizeof(datapath));
			sprintf(datapath, "C:\\Users\\Hank\\Desktop\\Data\\data%d", nIdx+1);
			pReadFile = fopen (datapath,"rb");
			if (pReadFile!=NULL)
			{
				fseek(pReadFile, 0, SEEK_END);   // non-portable
				nDataSize = ftell (pReadFile);
				rewind (pReadFile);
				
				pData = new unsigned char [nDataSize];
				nResult = fread (pData,sizeof(unsigned char),nDataSize,pReadFile);
				if (nResult == nDataSize)
				{
					//fwrite (Qtbl , sizeof(unsigned char), sizeof(Qtbl), pWriteFile);
					fwrite (pData , sizeof(unsigned char), nDataSize, pWriteFile);
				}

				fclose (pReadFile);
				delete [] pData;
			}
		}

		fclose (pWriteFile);
	}
}

void ConnectRTPServer()
{
	sockaddr_in rtpserver;
	int nResult = 0;
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	//char receive_message[1490] = {0};
	char receive_message[2048] = {0};
	int addr_len = sizeof(sockaddr_in);

	rtpserver.sin_family=AF_INET;
	// zack RTSP SETUP
	// send_message = "SETUP rtsp://10.1.21.34/rtpvideo1.sdp/track1 RTSP/1.0\r\nCSeq: 4\r\nTransport: RTP/AVP;unicast;client_port=55838-55839\r\n\r\n";
	// parse Response server port ??? to htons();
	
	// zack rtpserver.sin_port=htons(55838);
	rtpserver.sin_port=htons(55838);
	//rtpserver.sin_addr.s_addr=inet_addr("10.1.21.34");
	rtpserver.sin_addr.s_addr=INADDR_ANY;

	nResult = bind(sockfd,(sockaddr*)&rtpserver,sizeof(rtpserver));
	//nResult = connect(sockfd,(sockaddr*)(&rtpserver),sizeof(sockaddr));

	while(1)
	{
		nResult = recvfrom(sockfd, receive_message,sizeof(receive_message), 0 , (sockaddr*)&rtpserver ,&addr_len);
		printf(" %d\n", nResult);
	}
	closesocket(sockfd);
}

void ConnectRTSPServer()
{
	sockaddr_in webserver;
	int nResult = 0, sockfd = 0, addr_len = sizeof(sockaddr_in);
	char receive_message[1024] = {0};
	char chAuthEncode[30] = {0};

	int nPort = 554;
	char chIP[] = "10.1.21.34";

	sockfd = socket(AF_INET,SOCK_STREAM,0);
	webserver.sin_family=AF_INET;
	webserver.sin_port=htons(nPort);
	webserver.sin_addr.s_addr=inet_addr(chIP);

	string send_message;
	nResult = connect(sockfd,(sockaddr*)(&webserver),sizeof(sockaddr));
	for (int nIdx = 0; nIdx < 4; nIdx++)
	{
		if(nIdx == 0)
			send_message = "OPTIONS rtsp://10.1.21.34:554/rtpvideo1.sdp RTSP/1.0\r\nCSeq: 2\r\n\r\n";
		else if (nIdx == 1)
			send_message = "DESCRIBE rtsp://10.1.21.34:554/rtpvideo1.sdp RTSP/1.0\r\nCSeq: 3\r\nAccept: application/sdp\r\n\r\n";
		else if (nIdx == 2)
			send_message = "SETUP rtsp://10.1.21.34/rtpvideo1.sdp/track1 RTSP/1.0\r\nCSeq: 4\r\nTransport: RTP/AVP;unicast;client_port=55838-55839\r\n\r\n";
		//else if (nIdx == 3)
		//	send_message = "SETUP rtsp://10.1.21.34/rtpvideo1.sdp/track2 RTSP/1.0\r\nCSeq: 5\r\nTransport: RTP/AVP;unicast;client_port=55840-55841\r\nSession: 638451ED\r\n\r\n";
		else if (nIdx == 3)
			send_message = "PLAY rtsp://10.1.21.34/rtpvideo1.sdp/ RTSP/1.0\r\nCSeq: 6\r\nSession: 638451ED\r\nRange: npt=0.000-\r\n\r\n";

		memset(receive_message, 0x00, 1024);
		nResult = send(sockfd,send_message.c_str(),send_message.length(), 0);
		nResult = recvfrom(sockfd, receive_message,sizeof(receive_message), 0 , (sockaddr*)&webserver ,&addr_len);
	}
	// zack maybe IP Camera will not send stream when socket close.
	// zack closesocket(sockfd);
	
	ConnectRTPServer();
	closesocket(sockfd);
}

int main(int argc, char* argv[])
{
	if (!AfxSocketInit())
	{
		return FALSE;
	}

	//ConnectRTSPServer();
	WriteHead();
	WriteData();
	return 0;
}



