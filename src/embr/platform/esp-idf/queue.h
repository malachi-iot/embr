#pragma once

#if ESP_PLATFORM
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#else
#include <queue.h>
#endif

// EXPERIMENTAL
// Using estd naming conventions
namespace embr { namespace freertos {
    
namespace wrapper {

struct queue_base
{
protected:
    QueueHandle_t h;

    inline queue_base(QueueHandle_t h) : h{h} {}
};

struct queue : queue_base
{
    queue(QueueHandle_t h) : queue_base{h} {}

public:
    static queue create(UBaseType_t uxQueueLength, UBaseType_t uxItemSize)
    {
        queue q{xQueueCreate(uxQueueLength, uxItemSize)};
        return q;
    }

    static queue create(UBaseType_t uxQueueLength, UBaseType_t uxItemSize,
        uint8_t* pucQueueStorageBuffer, StaticQueue_t* pxQueueBuffer)
    {
        queue q{xQueueCreateStatic(uxQueueLength, uxItemSize, pucQueueStorageBuffer, pxQueueBuffer)};
        return q;
    }

    void free() { vQueueDelete(h); }

    BaseType_t send_from_isr(const void* pvItemToQueue, BaseType_t* pxHigherPriorityTaskWoken)
    {
        return xQueueSendFromISR(h, pvItemToQueue, pxHigherPriorityTaskWoken);
    }

    BaseType_t receive(void* pvBuffer, TickType_t xTicksToWait)
    {
        return xQueueReceive(h, pvBuffer, xTicksToWait);
    }
};

}

template <class T, bool is_static = false>
struct queue;

template <class T>
struct queue<T, false>
{
    typedef T value_type;
    typedef value_type* pointer;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef estd::chrono::freertos_clock::duration duration;

    wrapper::queue q;

public:
    queue(unsigned queue_length) : q(wrapper::queue::create(queue_length, sizeof(value_type)))
    {

    }

    bool receive(pointer v, duration timeout)
    {
        return q.receive(v, timeout.count()) == pdTRUE;
    }

    BaseType_t send_from_isr(const_reference v, BaseType_t* pxHigherPriorityTaskWoken = nullptr)
    {
        return q.send_from_isr(&v, pxHigherPriorityTaskWoken);
    }
};

}}