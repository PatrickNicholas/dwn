#pragma once

struct module_t {
    void (*init)();
    void (*destroy)();
};

#define MODULE_ATTRIBUTE(id) __attribute__((used)) __attribute__((section(".module_" #id)))

#define DEFINE_MODULE(name, id, init_fn, destroy_fn)                     \
    static struct module_t module_##name##_##id MODULE_ATTRIBUTE(id) = { \
        .init = init_fn,                                                 \
        .destroy = destroy_fn,                                           \
    }

#define DEFINE_KERNEL_MODULE(name, init_fn, destroy_fn) \
    DEFINE_MODULE(name, kernel, init_fn, destroy_fn)

#define DEFINE_CONFIG_MODULE(name, init_fn, destroy_fn) \
    DEFINE_MODULE(name, config, init_fn, destroy_fn)

extern struct module_t module_kernel_start;
extern struct module_t module_kernel_end;
extern struct module_t module_config_start;
extern struct module_t module_config_end;
