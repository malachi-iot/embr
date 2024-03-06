# FEATURE_EMBR_WORD_STRICTNESS

# FEATURE_EMBR_ESP_TIMER_SCHEDULER

Defaults to OFF since timer scheduler depends on old deprecated esp-idf pre-gptimer API

# FEATURE_EMBR_LWIP_NETCONN_EVENT

Enables cts/rts style event signaling from streambuf -> istream/ostream
NOT READY
Will depend on FEATURE_ESTD_STREAMBUF_TRAITS, although in its current state
it does not