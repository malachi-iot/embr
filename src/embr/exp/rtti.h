namespace embr { namespace experimental {

struct module_info
{
    const char* name;
};

struct type_info
{
    const module_info& module;
    const char* name;
    const int id;
};

}}