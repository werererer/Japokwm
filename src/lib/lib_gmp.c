#include "lib/lib_gmp.h"

#include "translationLayer.h"

static const struct luaL_Reg gmp_meta[] =
{
    {"__gc", lib_gmp_gc},
    {"__tostring", lib_gmp_tostring},
    {NULL, NULL},
};

static const struct luaL_Reg gmp_static_methods[] =
{
    {"new", lib_gmp_new},
    {NULL, NULL},
};

static const struct luaL_Reg gmp_static_setter[] =
{
    {NULL, NULL},
};

static const struct luaL_Reg gmp_static_getter[] =
{
    {NULL, NULL},
};

static const struct luaL_Reg gmp_methods[] =
{
    {NULL, NULL},
};

static const struct luaL_Reg gmp_setter[] =
{
    {NULL, NULL},
};

static const struct luaL_Reg gmp_getter[] =
{
    {NULL, NULL},
};

void lua_load_gmp(lua_State *L)
{
    create_class(L,
            gmp_meta,
            gmp_static_methods,
            gmp_methods,
            gmp_setter,
            gmp_getter,
            CONFIG_GMP);

    create_static_accessor(L,
            "Gmp",
            gmp_static_methods,
            gmp_static_setter,
            gmp_static_getter);
}

mpfr_ptr check_gmp(lua_State *L, int argn)
{
    void *ud = luaL_checkudata(L, argn, CONFIG_GMP);
    luaL_argcheck(L, ud != NULL, argn, "`mpfr' expected");
    return (mpfr_ptr)ud;
}

static void create_lua_gmp(lua_State *L, mpfr_t num)
{
    mpfr_ptr mpfr = (mpfr_ptr)lua_newuserdata(L, sizeof(mpfr_t));
    mpfr_init2(mpfr, mpfr_get_prec(num));
    mpfr_set(mpfr, num, GMP_RNDN);

    luaL_setmetatable(L, CONFIG_GMP);
}

// meta
int lib_gmp_gc(lua_State *L)
{
    mpfr_ptr n = check_gmp(L, 1);
    mpfr_clear(n);
    return 0;
}

int lib_gmp_tostring(lua_State *L)
{
    mpfr_ptr n = check_gmp(L, 1);
    char data[255];
    mpfr_snprintf(data, 255, "%.*Rf", 10, n);
    lua_pushstring(L, data);
    return 1;
}

int lib_gmp_new(lua_State *L)
{
    double n = luaL_checknumber(L, 1);
    lua_pop(L, 1);

    mpfr_t num;
    mpfr_init(num);
    mpfr_set_d(num, n, GMP_RNDN);
    create_lua_gmp(L, num);

    return 1;
}


