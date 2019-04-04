
#pragma once

#include <string>
#include <exception>
#include <map>
#include <vector>
#include <typeinfo>
#include <functional>
#include <cassert>
#include <cmath>
#include <sstream>


namespace sic {



//
// Exceptions
//
class error : std::runtime_error {
public:
    error(const std::string& what) : std::runtime_error(what) {}
    error() : std::runtime_error("") {}     // XXX probably not a good idea
    virtual const char *id() const { return "error"; }
    std::string msg() const {
        return std::string(id()) + (what() ? ": " : "") + what();
    }
};

class undefined_name : public error {
public:
    undefined_name(const std::string& nm) : error(nm) {}
    virtual const char *id() const override { return "undefined_name"; }
};

class redefined_name : public error {
public:
    redefined_name(const std::string& nm) : error(nm) {}
    virtual const char *id() const override { return "redefined_name"; }
};

class bad_arg : public error {
public:
    bad_arg(const std::string& exp, const std::string &got) :
        error("Expecting " + exp + "; got " + got) {}
    virtual const char *id() const override { return "bad_arg"; }
};

class wrong_type : public error {
public:
    wrong_type(const char* want, std::string got) :
        error(std::string("Expecting type ") +want+ "(?); got value " + got){}
    virtual const char *id() const override { return "wrong_type"; }
};

class arg_count : public error {
public:
    arg_count(std::size_t want, std::size_t got) :
        error("Expecting " + std::to_string(want) + "; got " + std::to_string(got)) {}
    virtual const char *id() const override { return "arg_count"; }
};

class not_a_function : public error {
public: virtual const char *id() const override { return "not_a_function"; }
};

class fn_arg_mismatch : public error {
public: virtual const char *id() const override { return "fn_arg_mismatch"; }
};

class malformed_expr : public error {
public: virtual const char *id() const override { return "malformed_expr"; }
};

class syntax_error : public error {
public:
    syntax_error(const std::string& nm) : error(nm) {}
    virtual const char *id() const override { return "syntax_error"; }
};



class obj;
class pair;
class callable;
class context;

extern pair *reverse(pair *list);
extern std::size_t llen(pair *lst);
extern obj* eval(obj* expr, context* ar);
//extern obj* tl_eval(obj *expr);
extern std::string printstr(obj *o);
extern void each(obj *list, std::function<void(obj *)> actor);
extern pair* map(pair *list, std::function<obj*(obj *)> actor);
extern pair *vec2list(const std::vector<obj*>& vec);
extern obj* read(std::istream& in);
extern context *root_context();


extern pair * const nil;
extern pair * const null;

extern callable * const add;
extern callable * const quote;
extern callable * const sub;
extern callable * const mul;
extern callable * const divi;
extern callable * const print;

extern callable * const make_function;;
extern callable * const lambda;
extern callable * const set;
extern callable * const setq;
extern callable * const progn;

extern callable * const quote2;


//
// Support code
//

// Dynamic cast which asserts success
template<typename T>
T* dca(obj *o) {
    T* result = dynamic_cast<T*>(o);
    if (!result) {
        throw wrong_type(typeid(T).name(), printstr(o));
    }
    return result;
}


class context {
    std::map<std::string, obj*> items;
public:
    context * const parent;
    context(context &) = delete;
    context(context *p) : parent(p) {}

    bool has(const std::string& name) const { return items.count(name) > 0; }
    void define(const std::string& name, obj* value) {
        if (has(name)) { throw redefined_name(name); }
        items[name] = value;
    }

    // Store 'value' at 'name'; throws undefined_name if name has not
    // been defined.
    void set(const std::string& name, obj* value) {
        if(!has(name)) {
            if (!parent) { throw undefined_name(name); }
            parent->set(name, value);
        }
        items[name] = value;
    }

    // Like set, but will create 'name' if it's undefined IF this is
    // the toplevel context.
    void tl_set(const std::string& name, obj *value) {
        if (parent) { set(name, value); }
        items[name] = value;
    }

    bool canfind(const std::string& name) const {
        return has(name) || (parent && parent->has(name));
    }

    obj* get(const std::string& name) const {
        if (has(name))  { return items.at(name); }
        if (parent)     { return parent->get(name); }
        throw undefined_name(name);
    }

    context* root() {   // XXX const?
        return parent ? parent->root() : this;
    }
};

class obj {
public:
    virtual bool isAtom()       const { return true; }
    virtual bool isCallable()   const { return false; }
    virtual bool isTrue()       const { return true; }
    virtual bool isSymbol()     const { return false; }
    virtual bool isList()       const { return false; }
    virtual bool isMacro()      const { return false; }

    virtual std::string str()   const = 0;
};

class string : public obj {
public:
    const std::string contents;

    string(std::string s) : contents(s) {}
    virtual std::string str() const override { return contents; }
};

class symbol : public obj {
    inline static std::map<std::string, symbol*> symbols;

    symbol(std::string& v) : text(v) {}

public:
    const std::string text;
    virtual bool isSymbol()     const override { return true; }
    virtual std::string str()   const override { return text; }

    static symbol* intern(std::string s) {
        if (symbols.count(s) == 0) { symbols[s] = new symbol(s); }
        return symbols[s];
    }
};



class number : public obj {
public:
    const double val;
    number(long l) : val((double)l) {}
    number(double d) : val(d) {}
    virtual std::string str() const override {
        if (val == trunc(val)) { return std::to_string((long long )val); }
        return std::to_string(val);
    }
};

class pair : public obj {
public:
    obj * const car;
    obj * const cdr;

    pair(obj *a, obj *d) : car(a), cdr(d) {}
    virtual bool isAtom() const override { return false; }
    virtual bool isList() const override { return cdr == nil || cdr->isList(); }
//    pair *next() const { return dca<pair>(cdr); }
    virtual std::string str() const override {
        return std::string("(") + car->str() + "." + cdr->str() + ")";
    }
};

class nilClass : public pair {
    inline static nilClass *instance = nullptr;

    nilClass() : pair(this, this) {}
    
public:
    virtual std::string str() const override { return "nil"; }
    virtual bool isTrue() const override { return false; }

    static nilClass *getInstance() {
        if (!instance) { instance = new nilClass(); }
        return instance;
    }
};


class callable : public obj {
public:
    virtual bool isCallable() const override { return true; }
    virtual std::string str() const override {
        return isMacro ? std::string("<macro>") : std::string("<callable>");
    }
    const bool isMacro;

    callable(bool m) : isMacro(m) {}

    virtual obj* call(obj* actualArgs, context* outer) const = 0;
};

class function : public callable {
    pair *formals, *body;
    context *outer;
public:
    function(pair* f, pair* b, context *ctx, bool m)
        : callable(m), formals(f), body(b), outer(ctx) {}
    virtual obj* call(obj* actualArgs, context* outer) const override;
};


class builtin : public callable {
public:
    using Callback = std::function<obj*(std::vector<obj*>&, context*)>;

private:
    const Callback      code;
    const std::size_t   nargs;
    const bool          isVariadic;

public:
    builtin(Callback c, std::size_t na, bool isvar=false, bool ismacro=false) :
        callable(ismacro), code(c),  nargs(na), isVariadic(isvar) {}

    virtual obj* call(obj* actualArgs, context* outer) const override;
};


//
// Client Helpers
//

static inline obj* _w(std::string s)  { return new string(s); }
static inline obj* _w(int i)          { return new number((long)i); }
static inline obj* _w(long l)         { return new number(l); }
static inline obj* _w(double d)       { return new number(d); }
static inline obj* _w(obj *o)         { return o; }

static inline obj* $$(std::string s)  { return symbol::intern(s); }

// List construction
//static inline obj* $(void)            { return nil; }

template<typename T>
obj* $(T o)             { return new pair(_w(o), nil); }

template<typename T, typename... Objects>
obj* $(T car, Objects... cdr) {
    return new pair(_w(car), $(cdr...));
}


static inline obj* read(std::string text) {
    std::stringstream s(text, std::ios_base::in);
    return read(s);
}


};
