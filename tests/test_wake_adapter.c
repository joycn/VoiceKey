#define ESP_LOGE(...) ((void)0)
#define ESP_ERR_NOT_FOUND 0x105
#define ESP_ERR_NOT_SUPPORTED 0x106
#define ESP_ERR_INVALID_SIZE 0x104
#include "../firmware/main/wake_engine.c"
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
static jmp_buf finished;
static bool missing_model, wrong_model;
static unsigned observations;
static unsigned reads,creates,destroys,detections,triggers,delays;
static void (*worker)(void *);
static model_iface_data_t instances[2];
static model_iface_data_t *create(const void *name,int mode) {
 (void)name;(void)mode;creates++;if(creates==2)return NULL;
 return &instances[creates==1 ? 0:1];
}
static void destroy(model_iface_data_t *m) {assert(m==&instances[0]);destroys++;}
static void clean(model_iface_data_t *m) {(void)m;assert(!"clean must not be called");}
static int detect(model_iface_data_t *m,int16_t *samples) {
 (void)samples;assert(m==&instances[detections==0 ? 0:1]);detections++;return WAKENET_DETECTED;
}
static int rate(model_iface_data_t *m) {(void)m;return 16000;}
static int channels(model_iface_data_t *m) {(void)m;return 1;}
static int chunks(model_iface_data_t *m) {(void)m;return 512;}
static const char *word(model_iface_data_t *m,int n) {(void)m;(void)n;return "你好小智";}
static const esp_wn_iface_t iface={create,destroy,clean,detect,rate,channels,chunks,word};
const esp_wn_iface_t *esp_wn_handle_from_name(char *name) {(void)name;return &iface;}
srmodel_list_t *esp_srmodel_init(const char *name) {(void)name;static srmodel_list_t m;return &m;}
char *esp_srmodel_filter(srmodel_list_t *m,const char *p,const char *f) {(void)m;assert(!strcmp(p,ESP_WN_PREFIX) && !strcmp(f,"nihaoxiaozhi_tts"));return missing_model ? NULL : wrong_model ? "wn9_hiesp" : "wn9_nihaoxiaozhi_tts";}
int xTaskCreatePinnedToCore(void (*fn)(void *),const char *n,uint32_t stack,void *a,unsigned pri,void *h,int core) {
 (void)n;(void)stack;(void)a;(void)pri;(void)h;(void)core;worker=fn;return pdPASS;
}
void vTaskDelay(TickType_t ticks) {assert(ticks==5 || ticks==100);delays++;}
size_t vk_runtime_wake_read(int16_t *samples,size_t count,uint32_t *epoch) {
 for(unsigned i=0;i<count;i++)samples[i]=i==0?INT16_MIN:100;assert(count==512);unsigned step=reads++;
 if(step==6)longjmp(finished,1);
 *epoch=step<2 ? 1:2;
 if(step==0 || step==2)return 0;
 return 512;
}
void vk_runtime_wake_observed(uint32_t epoch,uint16_t peak,bool detected) {assert(epoch==1 || epoch==2);assert(peak==32768 && detected);observations++;}
void vk_runtime_trigger(uint32_t epoch) {assert(epoch==(triggers==0 ? 1:2));triggers++;}
int main(void) {
 missing_model=true; assert(vk_wake_start()==ESP_ERR_NOT_FOUND && !worker);
 missing_model=false; wrong_model=true; assert(vk_wake_start()==ESP_ERR_NOT_FOUND && !worker);
 wrong_model=false; assert(vk_wake_start()==ESP_OK && worker);
 if(!setjmp(finished))worker(NULL);
 assert(creates==3 && destroys==1 && detections==3 && triggers==3 && delays==3);
 assert(observations==3);
 free(frame);
 puts("PASS: production wake task skips clean, recreates only used epoch state, retries failed recreation without stale inference");
}
