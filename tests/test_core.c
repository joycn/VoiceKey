#include "voicekey_core.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
static vk_state_t s;
static void ready(void) {
    vk_init(&s, 2000);
    vk_set_audio_ok(&s, true);
    vk_set_connected(&s, true);
    vk_set_streaming(&s, true);
    assert(vk_hid_next(&s, 0) == VK_HID_RELEASE);
    vk_hid_commit(&s, VK_HID_RELEASE, 0);
}
static void test_fifo(void) {
    int16_t storage[4], in[] = {1,2,3,4,5,6,7}, out[8];
    vk_fifo_t q;
    vk_fifo_init(&q, storage, 4);
    assert(vk_fifo_push(&q, in, 3) == 0);
    assert(vk_fifo_pop(&q, out, 2) == 2 && out[1] == 2);
    assert(vk_fifo_push(&q, in + 3, 3) == 0);
    assert(vk_fifo_pop(&q, out, 4) == 4);
    assert(out[0] == 3 && out[3] == 6);
    vk_fifo_push(&q, in, 3);
    assert(vk_fifo_push(&q, in + 3, 3) == 2);
    assert(vk_fifo_pop(&q, out, 4) == 4 && out[0] == 3 && out[3] == 6);
    assert(vk_fifo_push(&q, in, 7) == 3);
    assert(vk_fifo_pop(&q, out, 8) == 4 && out[0] == 4 && out[3] == 7);
    assert(q.dropped == 5);
    vk_fifo_init(&q, storage, 0);
    assert(vk_fifo_push(&q, in, 3) == 3);
}
static void test_pcm(void) {
    assert(vk_pcm_convert(INT32_MAX, 1) == INT16_MAX);
    assert(vk_pcm_convert(INT32_MIN, 1) == INT16_MIN);
    assert(vk_pcm_convert(65536, 2) == 2);
    assert(vk_pcm_convert(-65536, 2) == -2);
    assert(vk_pcm_convert(INT32_MAX, 8) == INT16_MAX);
    assert(vk_pcm_convert(INT32_MIN, 8) == INT16_MIN);
}
static void test_independent_audio(void) {
    ready();
    int16_t in[160], out[160], wake[160];
    for (int i=0;i<160;i++) in[i] = i + 1;
    vk_publish(&s, in, 160, s.audio_epoch);
    assert(vk_usb_read(&s, out, 160) == 160);
    uint32_t epoch;
    assert(vk_wake_read(&s, wake, 160, &epoch) == 160);
    assert(!memcmp(out, wake, sizeof(out)));
    assert(vk_usb_read(&s, out, 160) == 0 && s.usb.missing == 160);
    for (int i=0;i<160;i++) assert(out[i] == 0);
    vk_set_streaming(&s, false);
    for (int i=0;i<100;i++) vk_publish(&s, in, 160, s.audio_epoch);
    assert(s.usb.count == 0 && s.wake.count <= VK_WAKE_CAPACITY && s.wake.dropped > 0);
    vk_set_streaming(&s, true);
    assert(vk_usb_read(&s, out, 160) == 0);
    in[0] = 777;
    vk_publish(&s, in, 160, s.audio_epoch);
    assert(vk_usb_read(&s, out, 160) == 160 && out[0] == 777);
}
static void test_mute_generations(void) {
    ready();
    int16_t in[16] = {1234}, out[16];
    uint32_t a = s.audio_epoch, w = s.wake_epoch;
    vk_publish(&s, in, 16, a);
    assert(vk_request_trigger(&s, 100, w));
    vk_set_mute(&s, true);
    assert(s.usb.count == 0 && s.wake.count == 0 && !s.pending);
    vk_publish(&s, in, 16, a);
    assert(vk_usb_read(&s, out, 16) == 0);
    assert(!vk_request_trigger(&s, 101, s.wake_epoch));
    vk_set_mute(&s, false);
    assert(!vk_request_trigger(&s, 102, w));
    vk_publish(&s, in, 16, a); /* pre-mute DMA block returned late */
    assert(s.usb.count == 0 && s.wake.count == 0);
    vk_publish(&s, in, 16, s.audio_epoch);
    assert(vk_usb_read(&s, out, 16) == 16 && out[0] == 1234);
}
static void test_host_mute_independent(void) {
    ready();
    int16_t in[32] = {99}, out[32];
    vk_set_host_mute(&s, true);
    vk_publish(&s, in, 32, s.audio_epoch);
    assert(vk_usb_read(&s, out, 32) == 0 && s.wake.count == 32);
    assert(vk_request_trigger(&s, 100, s.wake_epoch));
    vk_set_mute(&s, true);
    vk_set_host_mute(&s, false);
    assert(s.muted && !s.pending); /* host cannot unmute physical switch */
}
static void test_hid(void) {
    ready();
    assert(vk_request_trigger(&s, 100, s.wake_epoch));
    assert(!vk_request_trigger(&s, 101, s.wake_epoch));
    assert(vk_hid_next(&s, 100) == VK_HID_PRESS);
    /* Endpoint not ready: do not commit. The queued key has not been sent. */
    assert(!s.key_down);
    vk_hid_commit(&s, VK_HID_PRESS, 110);
    assert(vk_hid_next(&s, 139) == VK_HID_NONE);
    assert(vk_hid_next(&s, 140) == VK_HID_RELEASE);
    vk_hid_commit(&s, VK_HID_RELEASE, 140);
    assert(!vk_request_trigger(&s, 2109, s.wake_epoch));
    assert(vk_request_trigger(&s, 2110, s.wake_epoch));
    vk_hid_commit(&s, VK_HID_PRESS, 2110);
    vk_set_mute(&s, true);
    assert(vk_hid_next(&s, 2111) == VK_HID_RELEASE);
    vk_hid_commit(&s, VK_HID_RELEASE, 2111);
    assert(vk_hid_next(&s, 2112) == VK_HID_NONE);
}
static void test_disconnect_and_expiry(void) {
    ready();
    uint32_t epoch = s.wake_epoch;
    assert(vk_request_trigger(&s, 1, epoch));
    vk_set_connected(&s, false);
    assert(vk_hid_next(&s, 100) == VK_HID_NONE && !s.pending);
    assert(!vk_request_trigger(&s, 101, s.wake_epoch));
    vk_set_connected(&s, true);
    assert(vk_hid_next(&s, 102) == VK_HID_RELEASE);
    vk_hid_commit(&s, VK_HID_RELEASE, 102);
    assert(vk_hid_next(&s, 103) == VK_HID_NONE);
    assert(!vk_request_trigger(&s, 104, epoch));
    assert(vk_request_trigger(&s, 105, s.wake_epoch));
    assert(vk_hid_next(&s, 306) == VK_HID_NONE && !s.pending);
    assert(vk_request_trigger(&s, 400, s.wake_epoch));
    vk_hid_commit(&s, VK_HID_PRESS, 400);
    vk_set_connected(&s, false);
    vk_set_connected(&s, true);
    assert(vk_hid_next(&s, 401) == VK_HID_RELEASE && !s.pending);
}
static void test_overflow_invalidates_inference(void) {
    ready();
    uint32_t epoch = s.wake_epoch;
    int16_t in[160] = {0};
    for (int i=0;i<30;i++) vk_publish(&s, in, 160, s.audio_epoch);
    assert(s.wake_epoch != epoch && !vk_request_trigger(&s, 100, epoch));
    assert(s.usb.count == VK_USB_CAPACITY);
    assert(vk_usb_packet_samples(&s) == 17);
    vk_set_audio_ok(&s, false);
    assert(vk_usb_packet_samples(&s) == 16);
    assert(!vk_request_trigger(&s, 200, s.wake_epoch));
}
static void test_fifo_randomized(void) {
    int16_t storage[17], reference[17], input[40], output[40];
    size_t used = 0;
    vk_fifo_t q; vk_fifo_init(&q, storage, 17);
    uint32_t random = 7;
    for (unsigned iter=0;iter<50000;iter++) {
        random = random * 1664525u + 1013904223u;
        size_t n = (random >> 16) % 40;
        if (random & 1) {
            for (size_t i=0;i<n;i++) input[i] = (int16_t)(iter+i);
            vk_fifo_push(&q,input,n);
            for (size_t i=0;i<n;i++) {
                if (used == 17) { memmove(reference,reference+1,16*sizeof(*reference)); used--; }
                reference[used++]=input[i];
            }
        } else {
            size_t want = n < used ? n : used;
            assert(vk_fifo_pop(&q,output,n) == want);
            assert(!memcmp(output,reference,want*sizeof(*output)));
            memmove(reference,reference+want,(used-want)*sizeof(*reference)); used-=want;
        }
        assert(q.count == used);
    }
}
static void test_clock_drift(void) {
    for (int drift=-1;drift<=1;drift++) {
        ready();
        int16_t in[161] = {0}, out[17];
        uint64_t missing_after_start = 0;
        for (unsigned ms=0;ms<60000;ms++) {
            if (ms % 10 == 0) {
                size_t count = 160;
                if (ms % 100 == 0) count = (size_t)(160 + drift);
                vk_publish(&s,in,count,s.audio_epoch);
                /* Drain wake input independently to isolate USB clock behavior. */
                vk_fifo_clear(&s.wake);
            }
            vk_usb_read(&s,out,vk_usb_packet_samples(&s));
            if (ms == 1000) missing_after_start = s.usb.missing;
        }
        assert(s.usb.dropped == 0);
        assert(s.usb.missing == missing_after_start);
    }
}
int main(void) {
    test_fifo(); test_pcm(); test_independent_audio(); test_mute_generations();
    test_host_mute_independent(); test_hid(); test_disconnect_and_expiry();
    test_overflow_invalidates_inference(); test_fifo_randomized(); test_clock_drift();
    puts("PASS: 10 core test groups, including 50,000 randomized FIFO operations");
}
