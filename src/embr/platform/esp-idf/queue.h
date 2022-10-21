#pragma once

#include <estd/array.h>

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

public:
    ESTD_CPP_CONSTEXPR_RET operator QueueHandle_t () const { return h; }
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

#if ( configQUEUE_REGISTRY_SIZE > 0 )
    const char* get_name() const { return pcQueueGetName(h); }
#endif

    void reset() { xQueueReset(h); }

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

namespace internal {

template <class T>
struct queue
{
    typedef T value_type;
    typedef value_type* pointer;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef estd::chrono::freertos_clock::duration duration;

protected:
    wrapper::queue q;

    queue(QueueHandle_t q) : q(q) {}
    ~queue()
    {
        q.free();
    }

public:
    bool receive(pointer v, duration timeout)
    {
        return q.receive(v, timeout.count()) == pdTRUE;
    }

    BaseType_t send_from_isr(const_reference v, BaseType_t* pxHigherPriorityTaskWoken = nullptr)
    {
        return q.send_from_isr(&v, pxHigherPriorityTaskWoken);
    }
};

}

template <class T, bool is_static = false>
class queue;

template <class T>
class queue<T, false> : public internal::queue<T>
{
    typedef internal::queue<T> base_type;
    typedef typename base_type::value_type value_type;

public:
    queue(unsigned queue_length) :
        base_type(
            wrapper::queue::create(queue_length, sizeof(value_type)))
    {

    }
};

template <class T>
class queue<T, true> : public internal::queue<T>
{
    StaticQueue_t static_queue;

    typedef internal::queue<T> base_type;
    typedef typename base_type::value_type value_type;
    typedef typename base_type::pointer pointer;

public:
    queue(pointer storage, unsigned queue_length) : 
        base_type(wrapper::queue::create(
            queue_length,
            sizeof(value_type),
            reinterpret_cast<uint8_t*>(storage),
            &static_queue))
    {

    }
};

namespace layer1 {

template <class T, unsigned queue_length>
class queue : public freertos::queue<T, true>
{
    typedef freertos::queue<T, true> base_type;

    estd::array<T, queue_length> storage;

public:
    queue() : base_type(storage.data(), queue_length) {}
};

}


}}