#include "Kbvison_cam.h"
#include <stdio.h>
#include <string.h>
#include "stdlib.h"
#include "stdint.h"
#include "socket.h"
#include "wizchip_conf.h"
#include "define.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"

#include "lwip/sockets.h"
#include "esp_http_client.h"
#define CAM_DEBUG 1
#if CAM_DEBUG
    static const char *TAG = "[KBVISION]";
#endif

#define CAM_BUFFER 4096
#define GET_IMAGE_SOCK 0
#define SET_LOCATION_SOCK 1

uint8_t Cam_addr[4] = {192,168,1,108};
uint16_t port = 80;

extern _StationData StData;
extern _StationConfig StConfig;

static const char  *CAM_FRIST_REQUEST  =  "GET /cgi-bin/snapshot.cgi? HTTP/1.1\r\n"
										"Host: 192.168.1.108\r\n"
										"Connection: keep-alive\r\n"
										"Upgrade-Insecure-Requests: 1\r\n"
										"User-Agent: Esp-idf\r\n"
										"Accept: */*\rUpgrade\n"
										"Accept-Encoding: gzip, deflate\r\n"
										"Accept-Language: en-US\r\n\r\n";
static const char  *CAM_SECON_REQUEST =	"GET /cgi-bin/snapshot.cgi? HTTP/1.1\r\n"
										"Host: 192.168.1.108\r\n"
										"Connection: keep-alive\r\n"
										"Cache-Control: max-age=0\r\n";
static const char  *CAM_THR_REQUEST =	"Upgrade-Insecure-Requests: 1\r\n"
										"Esp-idf\r\n"
										"Accept: */*\r\n"
										"Accept-Encoding: gzip, deflate\r\n"
										"Accept-Language: en-US\r\n"
										"Cookie: secure\r\n\r\n";
uint32_t h0, h1, h2, h3;
char nonce[20];
char realm[64];
uint32_t nc= 1;
char cnonce[17];
char *user = "admin";
char *pass = "namlong2018";
char *method="GET";
char *URI = "/cgi-bin/snapshot.cgi?";
char response[64];

#define LEFTROTATE(x, c) (((x) << (c)) | ((x) >> (32 - (c))))
static void md5(uint8_t *initial_msg, size_t initial_len) {

    // Message (to prepare)
    uint8_t *msg = NULL;

    // Note: All variables are unsigned 32 bit and wrap modulo 2^32 when calculating

    // r specifies the per-round shift amounts

    uint32_t r[] = {7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
                    5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20,
                    4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
                    6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21};

    // Use binary integer part of the sines of integers (in radians) as constants// Initialize variables:
    uint32_t k[] = {
        0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
        0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
        0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
        0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
        0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
        0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
        0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
        0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
        0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
        0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
        0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
        0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
        0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
        0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
        0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
        0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391};

    h0 = 0x67452301;
    h1 = 0xefcdab89;
    h2 = 0x98badcfe;
    h3 = 0x10325476;

    // Pre-processing: adding a single 1 bit
    //append "1" bit to message
    /* Notice: the input bytes are considered as bits strings,
       where the first bit is the most significant bit of the byte.[37] */

    // Pre-processing: padding with zeros
    //append "0" bit until message length in bit ≡ 448 (mod 512)
    //append length mod (2 pow 64) to message

    int new_len = ((((initial_len + 8) / 64) + 1) * 64) - 8;

    msg = calloc(new_len + 64, 1); // also appends "0" bits
                                   // (we alloc also 64 extra bytes...)Request_final
    memcpy(msg, initial_msg, initial_len);
    msg[initial_len] = 128; // write the "1" bit

    uint32_t bits_len = 8*initial_len; // note, we append the len
    memcpy(msg + new_len, &bits_len, 4);           // in bits at the end of the buffer

    // Process the message in successive 512-bit chunks:
    //for each 512-bit chunk of message:
    int offset;
    for(offset=0; offset<new_len; offset += (512/8)) {

        // break chunk into sixteen 32-bit words w[j], 0 ≤ j ≤ 15
        uint32_t *w = (uint32_t *) (msg + offset);

#ifdef DEBUG
        printf("offset: %d %x\n", offset, offset);

        int j;
        for(j =0; j < 64; j++) printf("%x ", ((uint8_t *) w)[j]);
        puts("");
#endif

        // Initialize hash value for this chunk:
        uint32_t a = h0;
        uint32_t b = h1;
        uint32_t c = h2;
        uint32_t d = h3;

        // Main loop:
        uint32_t i;
        for(i = 0; i<64; i++) {

#ifdef ROUNDS
            uint8_t *p;
            printf("%i: ", i);
            p=(uint8_t *)&a;
            printf("%2.2x%2.2x%2.2x%2.2x ", p[0], p[1], p[2], p[3], a);

            p=(uint8_t *)&b;
            printf("%2.2x%2.2x%2.2x%2.2x ", p[0], p[1], p[2], p[3], b);

            p=(uint8_t *)&c;
            printf("%2.2x%2.2x%2.2x%2.2x ", p[0], p[1], p[2], p[3], c);

            p=(uint8_t *)&d;
            printf("%2.2x%2.2x%2.2x%2.2x", p[0], p[1], p[2], p[3], d);
            puts("");
#endif


            uint32_t f, g;

             if (i < 16) {
                f = (b & c) | ((~b) & d);
                g = i;
            } else if (i < 32) {
                f = (d & b) | ((~d) & c);
                g = (5*i + 1) % 16;
            } else if (i < 48) {
                f = b ^ c ^ d;
                g = (3*i + 5) % 16;
            } else {
                f = c ^ (b | (~d));
                g = (7*i) % 16;
            }

#ifdef ROUNDS
            printf("f=%x g=%d w[g]=%x\n", f, g, w[g]);
#endif
            uint32_t temp = d;
            d = c;
            c = b;
//            printf("rotateLeft(%x + %x + %x + %x, %d)\n", a, f, k[i], w[g], r[i]);
            b = b + LEFTROTATE((a + f + k[i] + w[g]), r[i]);
            a = temp;
        }

        // Add this chunk's hash to result so far:

        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;

    }

    // cleanup
    free(msg);

}
static void Md5_caculator(char *res,uint8_t *initial_msg, size_t initial_len)
{

    md5(initial_msg, initial_len);
    // }
    //var char digest[16] := h0 append h1 append h2 append h3 //(Output is in little-endian)
    uint8_t *p;
    // display result
    p=(uint8_t *)&h0;
    sprintf(res,"%2.2x%2.2x%2.2x%2.2x", p[0], p[1], p[2], p[3]);
    p=(uint8_t *)&h1;
 //   printf("%2.2x%2.2x%2.2x%2.2x", p[0], p[1], p[2], p[3], h1);
    sprintf(res+8,"%2.2x%2.2x%2.2x%2.2x", p[0], p[1], p[2], p[3]);
    p=(uint8_t *)&h2;
//    printf("%2.2x%2.2x%2.2x%2.2x", p[0], p[1], p[2], p[3], h2);
    sprintf(res+16,"%2.2x%2.2x%2.2x%2.2x", p[0], p[1], p[2], p[3]);
    p=(uint8_t *)&h3;
//    printf("%2.2x%2.2x%2.2x%2.2x", p[0], p[1], p[2], p[3], h3);
    sprintf(res+24,"%2.2x%2.2x%2.2x%2.2x", p[0], p[1], p[2], p[3]);
//    printf("%s\n",res);
}
static int Get_nonce(char *src)
{
	char *src1,*src2;
	src1 = strstr (src,"nonce"); //cắt chuỗi từ content trở đi => str1
	if(src1 == 0)
		return -1;
	src2= strstr (src1+8,"\""); //cắt chuỗi từ \r\n(kết thúc của 1 key header) trở đi => str2
	int len1,len2,len3;
	len1 = strlen(src1);
	len2 = strlen(src2);
	len3 = strlen("nonce");
	int size_nonce = len1-len2-len3-2;
    strncpy(nonce,src1+len3+2,size_nonce);
    nonce[size_nonce]=0;
	//printf("nonce: %s\n",nonce );
	return 0;
}
static int Get_realm(char *src)
{
	char *src1,*src2;
	src1 = strstr (src,"realm"); //cắt chuỗi từ content trở đi => str1
	if(src1 == 0)
		return -1;
	src2= strstr (src1+8,"\""); //cắt chuỗi từ \r\n(kết thúc của 1 key header) trở đi => str2
	int len1,len2,len3;
	len1 = strlen(src1);
	len2 = strlen(src2);
	len3 = strlen("nonce");
	int size_realm= len1-len2-len3-2;
    strncpy(realm,src1+len3+2,size_realm);
    realm[size_realm]=0;
	//printf("nonce: %s\n",nonce );
	return 0;
}
static void Response_Caculator(void)
{
		char buf[1024];
		int len = sprintf(buf,"%s:%s:%s",user,realm,pass);
	    char HA1[256],HA2[256];
	    Md5_caculator(HA1,(uint8_t *)buf,len);
	    len = sprintf(buf,"%s:%s",method,URI);
	    //printf("%s\n",buf );
	    Md5_caculator(HA2,(uint8_t *)buf,len);
	    len = sprintf(buf,"%s:%s:%08d:%s:%s:%s",HA1,nonce,nc,cnonce,"auth",HA2);
	    //printf("total: %s\n",buf );
	    Md5_caculator(response,(uint8_t *)buf,len);
	    //printf("%s\n",response );
}
static int Get_Data_String(char *src,char *content,char *des)
{
	char *src1,*src2;
	src1 = strstr (src,content); //cắt chuỗi từ content trở đi => str1
	if(src1 == 0)
		return -1;
	src2= strstr (src1,"\r\n"); //cắt chuỗi từ \r\n(kết thúc của 1 key header) trở đi => str2
	int len1,len2,len3;
	len1 = strlen(src1);
	len2 = strlen(src2);
	len3 = strlen(content);
    //printf(" *(src1+len3+2) :0x%X ,0x%X\n", src1[len3+1],src1[len3+2] );
    if(src1[len3+1] == 0x20)
	    {//	if(!wizphy_getphylink())
    	//	{
    	//		#if CAM_DEBUG
    	//        	ESP_LOGE(TAG,"The cable is not plugged in");
    	//        #endif
    	//        return -11;
    	//	}
            strncpy(des,src1+len3+2,(len1-len2-len3-2));
        } //cắt chỗi từ str1 kể từ vị trí độ dài contern + 2( : và cách )
    else
        {
            strncpy(des,src1+len3+1,(len1-len2-len3-1));
        } //cắt chỗi từ str1 kể từ vị trí độ dài contern + 1( : và)

	//printf(" get : %s, len :%d =>%sketthuc\n", content ,(len1-len2-2),des );
	return 0;
}
static int Get_Content_length(char *str)
{
    char tmp[20];
    memset(tmp,0,20);
    if(Get_Data_String(str,"CONTENT-LENGTH",tmp) == 0)
        return atoi(tmp);
    return -1;
}
static int Make_requset(char *Request_Buff)
{
	int len =  sprintf(Request_Buff,"%sAuthorization: Digest username=\"admin\", realm=\"%s\", nonce=\"%s\", uri=\"/cgi-bin/snapshot.cgi?\", response=\"%s\", opaque=\"2420ce8142cb88faf488830c430b9f8635d3785c\", qop=auth, nc=%08d, cnonce=\"%s\"\r\n%s",CAM_SECON_REQUEST,realm,nonce,response,nc,cnonce,CAM_THR_REQUEST);
	//printf("%s\n",Request_Buff);
	return len;
}


int Update_image(esp_http_client_handle_t client,char *Image_name)
{
	if(!wizphy_getphylink())
	{
		#if CAM_DEBUG
        	ESP_LOGE(TAG,"The cable is not plugged in");
        #endif
        return -11;
	}
	nc++;
	sprintf(cnonce,"%06d%010d",StConfig.ID,(uint32_t)Get_mili());
	uint8_t Buffer_t[CAM_BUFFER+1];
	int image_size;
	int Image_index=0;
    /**/
	int res=-1111;
	int tot=0;
	int send=0;

	if(esp_http_client_set_header(client,"Filename",Image_name)!= ESP_OK)
	{
            ESP_LOGE(TAG,"esp_http_client_set_header fail");
    }
	
    int socketID = eth_socket(0,Sn_MR_TCP,50612, 0);
    if(socketID ==0)
    {
        int cn = eth_socket_connect(0,Cam_addr,port);
        if(cn == SOCK_OK )
        {
        	//-----gui 1 request de lay chuoi bao mat
            int len = eth_socket_send(0,(uint8_t *)CAM_FRIST_REQUEST,strlen(CAM_FRIST_REQUEST));
            if(len == strlen(CAM_FRIST_REQUEST))
            {
                len = eth_socket_recv(0,Buffer_t,CAM_BUFFER);
                if(len >0)
                {
                	Buffer_t[len] =0;
                    Get_nonce((char *)Buffer_t);
                    Get_realm((char *)Buffer_t);
                    Response_Caculator();
                    image_size = Make_requset((char *)Buffer_t);
                    cn = eth_socket_close(0);
                    if(cn == SOCK_OK )
                    {
                    	socketID = eth_socket(0,Sn_MR_TCP,50613, 0);
                    	if(socketID ==0)
                    	{
							cn = eth_socket_connect(0,Cam_addr,port);
							if(cn == SOCK_OK )
							{
								len = eth_socket_send(0,Buffer_t,image_size);
							}
                    	}
                    }
                    image_size =-1;
                    while(tot!= image_size)
                    {
                    	    len = eth_socket_recv(0,Buffer_t,CAM_BUFFER);
                            if(len>0)
                            {
                                if(image_size <0)
                                {
                                	//goi du lieu dau tien bao gom header va image data
                                	image_size = Get_Content_length((char *)Buffer_t);
                                    #if CAM_DEBUG
                                                    ESP_LOGI(TAG,"Image size: %d byte",image_size );
                                    #endif
                                    char *src1=NULL;
                                    src1 = strstr((char *)Buffer_t,"\r\n\r\n"); //cắt chuỗi từ content trở đi => str1
                                    if(src1)
                                    {
                                        Image_index = strlen((char *)Buffer_t) - strlen(src1)+4;
										#if CAM_DEBUG
                                                   ESP_LOGI(TAG,"Image index: %d ",Image_index );
										#endif
                                        //esp_http_client_write(client,(const char *)Buffer_t+Image_index,len-Image_index)
                                        /*-----------------ftp------------------*/
										if(esp_http_client_open(client,image_size)== ESP_OK)
										{
											send = esp_http_client_write(client,(const char *)Buffer_t+Image_index,len-Image_index);
											tot+=send;
										}
                                        /*-----------------ftp------------------*/
                                    }
                                    else
                                    {
                                    	res = -101;
                                    	break;
                                    }
                                }
                                else
                                {
                                	/*-----------------ftp------------------*/
                                	send = esp_http_client_write(client,(const char *)Buffer_t,len);
                                	tot+=send;
                                	/*-----------------ftp------------------*/
                                }
								#if CAM_DEBUG
									ESP_LOGI(TAG,"Processing img: %d %c",(tot*100)/image_size,'%');
								#endif
                            }
                            else
                            {
                                break;
                            }

                    }
                    /*-----------------ftp------------------*/
                    if(tot== image_size)
                    {
                    	esp_http_client_fetch_headers(client);
						int status_code = esp_http_client_get_status_code(client);
						ESP_LOGI(TAG, "tot: %d byte,status_code: %d",tot,status_code);
						len = esp_http_client_read(client,(char *)Buffer_t,CAM_BUFFER);
						if(len >0)
						{
							Buffer_t[len] =0;
							#if CAM_DEBUG
								ESP_LOGI(TAG, "recv: %d byte:%s",len,Buffer_t);
							#endif
						}
						/*-----------------ftp------------------*/
						res= tot;
                    }
                    else{
						#if CAM_DEBUG
								ESP_LOGE(TAG,"tot: %d,image_size: %d,len: %d",tot,image_size,len);
						#endif
                    	res =-299;
                    }
                }
                else
                    res =-300;
            }
            else
                res=-400;
        }
        else
            res=-500;
    }
    else res = -600;
    esp_http_client_close(client);
//    esp_http_client_cleanup(client);

    if(eth_socket_close(0)!= SOCK_OK)
    {
		#if CAM_DEBUG
				ESP_LOGE(TAG,"eth_socket_close fail");
		#endif
    }
    return res;
}
