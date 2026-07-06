#include "ff.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "xaxidma.h"
#include "platform.h"
#include "xil_cache.h"
#include "xil_printf.h"
#include "xiltimer.h"
#include "xil_io.h"
#include "xparameters.h"

#define KMER_SIZE_BYTES   31
#define PADDING_BYTES     1
#define KMER_TOTAL_BYTES  (KMER_SIZE_BYTES + PADDING_BYTES) // 32 bytes (8 palabras de 32-bit)
#define NUM_KMERS_1       135365
#define NUM_KMERS_2       135262
#define BUFFER_SIZE_1   (NUM_KMERS_1 * KMER_TOTAL_BYTES)
#define BUFFER_SIZE_2   (NUM_KMERS_2 * KMER_TOTAL_BYTES)


#define N_COMPARE       ((NUM_KMERS_1 < NUM_KMERS_2) ? NUM_KMERS_1 : NUM_KMERS_2)
#define TX_BYTES        (N_COMPARE * KMER_TOTAL_BYTES)
#define RX_WORDS        ((N_COMPARE + 31) / 32)
#define RX_BYTES        (RX_WORDS * sizeof(u32))
#define DMA_0_ID        XPAR_XAXIDMA_0_BASEADDR
#define DMA_1_ID        XPAR_XAXIDMA_1_BASEADDR

XAxiDma AxiDma0;
XAxiDma AxiDma1;

FRESULT res;
FATFS fatfs;
FIL fil0, fil1, fil2;

u8 TxBuffer0[BUFFER_SIZE_1] __attribute__ ((aligned(32)));
u8 TxBuffer1[BUFFER_SIZE_2] __attribute__ ((aligned(32)));
static u32 RxBuffer[RX_WORDS] __attribute__ ((aligned(32)));


int init_sd_and_read_files() {
    UINT bytesRead;

    // 1. Montar SD
    res = f_mount(&fatfs, "0:", 1);
    if (res != FR_OK) return XST_FAILURE;

    // 2. Abrir archivos
    if (f_open(&fil0, "31mers_5.txt", FA_READ) != FR_OK) return XST_FAILURE;
    if (f_open(&fil1, "31mers_6.txt", FA_READ) != FR_OK) return XST_FAILURE;

    // 3. Leer y aplicar Padding
    for (int i = 0; i < NUM_KMERS_1; i++) {
        // Leer 31 bytes del archivo 0
        f_read(&fil0, &TxBuffer0[i * KMER_TOTAL_BYTES], KMER_SIZE_BYTES, &bytesRead);
        char dummy;
        f_read(&fil0, &dummy, 1, &bytesRead);
        TxBuffer0[(i * KMER_TOTAL_BYTES) + 31] = 0x00; // Padding
    }
    for (int i = 0; i < NUM_KMERS_2; i++) {
        // Leer 31 bytes del archivo 1
        f_read(&fil1, &TxBuffer1[i * KMER_TOTAL_BYTES], KMER_SIZE_BYTES, &bytesRead);
        char dummy;
        f_read(&fil1, &dummy,1, &bytesRead);
        TxBuffer1[(i * KMER_TOTAL_BYTES) + 31] = 0x00; // Padding
    }

    xil_printf("Archivos leidos.\n");
    f_close(&fil0);
    f_close(&fil1);

    return XST_SUCCESS;
}

typedef struct {
    unsigned char data[31];
    unsigned char dummy;
} kmer;


size_t get_max_stride(unsigned* a, size_t n);
void count_strides(unsigned* a, size_t n, unsigned* stride_count);
unsigned sw_validation();
int write_histogram_file(unsigned* histogram, size_t n);

int main() {
    XTime t_start0, t_start1, t_end;
    unsigned long long u_time0, u_time1, u_time2;

    xil_printf("Hello world.\r\n");

    init_platform();

    if (init_sd_and_read_files() != XST_SUCCESS) {
        xil_printf("Error al leer los archivos\r\n");
        return 1;
    }

////////////////////////////////////////////////////////////////////////////////
    XTime_GetTime(&t_start0);

    XAxiDma_Config *CfgPtr0; 
    XAxiDma_Config *CfgPtr1; 

	int status;

	CfgPtr0 = XAxiDma_LookupConfig(DMA_0_ID);
	if (!CfgPtr0) {
		xil_printf("No config found for DMA0\r\n");
		return XST_FAILURE;
	}
	CfgPtr1 = XAxiDma_LookupConfig(DMA_1_ID);
	if (!CfgPtr1) {
		xil_printf("No config found for DMA1\r\n");
		return XST_FAILURE;
	}

	status = XAxiDma_CfgInitialize(&AxiDma0, CfgPtr0);
	if (status != XST_SUCCESS) {
		xil_printf("DMA0 initialization failed %d\r\n", status);
		return XST_FAILURE;
	}
	status = XAxiDma_CfgInitialize(&AxiDma1, CfgPtr1);
	if (status != XST_SUCCESS) {
		xil_printf("DMA1 initialization failed %d\r\n", status);
		return XST_FAILURE;
	}

	if(XAxiDma_HasSg(&AxiDma0)){
		xil_printf("Device configured as SG mode \r\n");
		return XST_FAILURE;
	}
	if(XAxiDma_HasSg(&AxiDma1)){
		xil_printf("Device configured as SG mode \r\n");
		return XST_FAILURE;
	}

	XAxiDma_IntrDisable(&AxiDma0, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DEVICE_TO_DMA);
	XAxiDma_IntrDisable(&AxiDma0, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DMA_TO_DEVICE);
	XAxiDma_IntrDisable(&AxiDma1, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DMA_TO_DEVICE);
////////////////////////////////////////////////////////////////////////////////

    Xil_DCacheFlushRange((UINTPTR)TxBuffer0, TX_BYTES);
    Xil_DCacheFlushRange((UINTPTR)TxBuffer1, TX_BYTES);
    Xil_DCacheFlushRange((UINTPTR)RxBuffer, RX_BYTES);

    XTime_GetTime(&t_start1);

    status = XAxiDma_SimpleTransfer(&AxiDma0, (UINTPTR)RxBuffer, RX_BYTES, XAXIDMA_DEVICE_TO_DMA);
    if (status != XST_SUCCESS){
        xil_printf("DMA0 S2MM transfer failed...\r\n");
        return XST_FAILURE;
    }

    status = XAxiDma_SimpleTransfer(&AxiDma0, (UINTPTR)TxBuffer0, TX_BYTES, XAXIDMA_DMA_TO_DEVICE);
    if (status != XST_SUCCESS){
        xil_printf("DMA0 MM2S transfer failed...\r\n");
        return XST_FAILURE;
    }

    status = XAxiDma_SimpleTransfer(&AxiDma1, (UINTPTR)TxBuffer1, TX_BYTES, XAXIDMA_DMA_TO_DEVICE);
    if (status != XST_SUCCESS){
        xil_printf("DMA1 MM2S transfer failed...\r\n");
        return XST_FAILURE;
    }

    while (XAxiDma_Busy(&AxiDma0, XAXIDMA_DMA_TO_DEVICE) ||
           XAxiDma_Busy(&AxiDma1, XAXIDMA_DMA_TO_DEVICE) ||
           XAxiDma_Busy(&AxiDma0, XAXIDMA_DEVICE_TO_DMA)) {
    }

    Xil_DCacheInvalidateRange((UINTPTR)RxBuffer, RX_BYTES);

    // Validar que termino el PL
    // Polling in_status hasta que sea 1
    while(!Xil_In32(XPAR_AXIREGS_0_BASEADDR+4));
    unsigned matchers = Xil_In32(XPAR_AXIREGS_0_BASEADDR);
    XTime_GetTime(&t_end);
    u_time0 = (t_end-t_start0)*1000000/COUNTS_PER_SECOND;
    u_time1 = (t_end-t_start1)*1000000/COUNTS_PER_SECOND;

    unsigned* compared_kmers = (unsigned*)RxBuffer;
    size_t compared_kmers_size = RX_WORDS;

    // Create histogram
    size_t hist_size = get_max_stride(compared_kmers, compared_kmers_size) + 1;
    unsigned* histogram = (unsigned*)calloc(hist_size, sizeof(unsigned));
    count_strides(compared_kmers, compared_kmers_size, histogram);

    // Print results
    for (size_t i=0; i<hist_size; ++i) {
        if (histogram[i])
            xil_printf("Length %u = Count %u\r\n", i, histogram[i]);
    }

    // Sw validation
    XTime_GetTime(&t_start0);
    unsigned sw_matches = sw_validation();
    XTime_GetTime(&t_end);
    u_time2 = (t_end-t_start0)*1000000/COUNTS_PER_SECOND;
    ////////////////
    xil_printf("Number of matches from PL: %u\r\n", matchers);
    xil_printf("Number of matches from sw: %u\r\n", sw_matches);

    xil_printf("PL time =             %llu us\r\n", u_time0);
    xil_printf("PL time (no config) = %llu us\r\n", u_time1);
    xil_printf("PS time =             %llu us\r\n", u_time2);

    // Save histogram
    if(write_histogram_file(histogram, hist_size) != XST_SUCCESS) {
        xil_printf("Error al escribir el histograma.\r\n");
        return 1;
    }

    // Clean memory
    free(histogram);

    return 0;
}


size_t get_max_stride(unsigned* a, size_t n) {
    size_t max = 0;
    bool in_match = false;
    size_t count = 0;

    for (size_t i=0; i<n; ++i) {
        for (size_t j=0; j<32; ++j) {
            unsigned char data = (a[i]>>j) & 1;
            if (in_match && data) {
                ++count;
            } else if (in_match) {
                in_match = false;
                max = max < count ? count : max;
            } else if (data) {
                in_match = true;
                count = 1;
            } 
        }
    }

    return max;
}

void count_strides(unsigned* a, size_t n, unsigned* stride_count) {
    bool in_match = false;
    size_t count = 0;

    for (size_t i=0; i<n; ++i) {
        for (size_t j=0; j<32; ++j) {
            unsigned char data = (a[i]>>j) & 1;
            if (in_match && data) {
                ++count;
            } else if (in_match) {
                in_match = false;
                ++stride_count[count];
            } else if (data) {
                in_match = true;
                count = 1;
            } 
        }
    }
}

unsigned sw_validation() {
    size_t matches = 0;
    size_t n = BUFFER_SIZE_1 < BUFFER_SIZE_2 ? BUFFER_SIZE_1 : BUFFER_SIZE_2;

    for (size_t i = 0; i<n; i+=32) {
        if (memcmp(TxBuffer0 + i, TxBuffer1 + i, 31) == 0) ++matches;
    }

    return matches;
}


int write_histogram_file(unsigned* histogram, size_t n) {
    UINT bytesWritten;
    // 1. Abrir archivos
    if (f_open(&fil2, "hist.txt", FA_CREATE_ALWAYS | FA_WRITE) != FR_OK) return XST_FAILURE;

    // 2. Leer y aplicar Padding
    char s[10];
    char space = ' ', endl = '\n', colon = ':';
    for (size_t i=0, j; i<n; ++i) {
        itoa(i, s, 10);
        for (j=0; s[j]; ++j); // find null terminated char pos

        f_write(&fil2, s, j, &bytesWritten);
        f_write(&fil2, &space, 1, &bytesWritten);
        f_write(&fil2, &colon, 1, &bytesWritten);
        f_write(&fil2, &space, 1, &bytesWritten);

        itoa(histogram[i], s, 10);
        for (j=0; s[j]; ++j); // find null terminated char pos
        f_write(&fil2, s, j, &bytesWritten);
        f_write(&fil2, &endl, 1, &bytesWritten);
    }

    xil_printf("Archivo escrito.\r\n");
    f_close(&fil2);

    return XST_SUCCESS;
}
