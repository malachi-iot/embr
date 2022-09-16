#pragma once

namespace embr { namespace experimental {

// Similar to unique ptr, but used for RIAA on wrappers
// such as lwip::udp::Pcb
template <class T>
struct Unique;

}}