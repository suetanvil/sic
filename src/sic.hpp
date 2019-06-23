// This file is part of Sic; Copyright (C) 2019 The Author(s)
// LGPLv2 w/ exemption; NO WARRANTY! See Copyright.txt for details

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
class error : public std::runtime_error {
private:
    std::vector<std::string> backtraces;
public:
    error(const std::string& what) : std::runtime_error(what) {}
    error() : std::runtime_error("") {}     // XXX probably not a good idea
    virtual const char *id() const { return "error"; }

    void addtrace(const std::string& trace) { backtraces.push_back(trace); }
    std::string backtrace() const {
        std::string result = "";
        for (const std::string& s : backtraces) {
            result += "  > " + s + "\n";
        }
        return result;
    }

    std::string msg() const {
        return std::string(id()) + (what() ? ": " : "") + what();
    }

    std::string longmsg() const { return msg() + "\n" + backtrace(); }
        
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
    wrong_type(const std::string msg) : error(msg) {}
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

class assertion_failure : public error {
public:
    assertion_failure(const std::string& msg) : error(msg) {}
    assertion_failure(const std::string& want,
                      const std::string& got,
                      const std::string& msg) :
        error(msg + ": " + "Expecting '" + want + "'; got '" + got + "'") {}
    virtual const char *id() const override { return "assertion_failure"; }
};



class obj;
class pair;
class symbol;
class callable;
class context;

extern pair *reverse(pair *list);
extern std::size_t llen(obj *lst);
extern obj* eval(obj* expr, context* ctx);
extern std::string printstr(obj *o, const context *ctx = nullptr,
                            bool forDebugging = false);
extern void basic_each(obj *list, std::function<void(obj *)> actor);
extern pair* basic_map(pair *list, std::function<obj*(obj *)> actor);
extern obj *basic_nth(obj *list, int index);
extern pair *vec2list(const std::vector<obj*>& vec);
extern obj* read(std::istream& in);
extern context *root_context();
extern const char *po(obj *o);
extern const char *po2(obj *o, const context *ctx);

extern pair *     const nil;
extern pair *     const null;
extern symbol *   const t;      // The classic Lisp 'true' symbol.


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
    // the toplevel context.  Convenience method.
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

    // Reverse lookup. Slow.
    std::string name_of(const obj* o) const {
        for (const auto& item : items) {
            if (item.second == o) { return item.first; }
        }

        if (parent) { return parent->name_of(o); }

        return "";
    }// o) 
    
    context *root() {
        return parent ? parent->root() : this;
    }
};

class obj {
public:
    virtual bool isAtom()       const { return true; }
    virtual bool isCallable()   const { return false; }
    virtual bool isTrue()       const { return true; }
    virtual bool isSymbol()     const { return false; }
    virtual bool isString()     const { return false; }    
    virtual bool isList()       const { return false; }
    virtual bool isMacro()      const { return false; }
    virtual bool equals(obj *o) const { return o == this; }

    virtual std::string str()   const = 0;
};

class string : public obj {
public:
    const std::string contents;

    explicit string(std::string s) : contents(s) {}
    virtual bool isString()     const override { return true; }
    virtual bool equals(obj* o) const override {
        string *os = dynamic_cast<string*>(o);
        return os && os->contents == contents;
    }
    virtual std::string str() const override { return contents; }
};

class symbol : public obj {
private:
    inline static std::map<std::string, symbol*> symbols;

    explicit symbol(std::string& v) : text(v) {}

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
    
    explicit number(long l) : val((double)l) {}
    explicit number(double d) : val(d) {}
    
    virtual std::string str() const override {
        if (val == trunc(val)) { return std::to_string((long long )val); }
        return std::to_string(val);
    }

    virtual bool equals(obj* o) const override {
        number *on = dynamic_cast<number*>(o);
        return on && on->val == val;
    }
    
};

class pair : public obj {
public:
    obj * const first;
    obj * const rest;

    explicit pair(obj *a, obj *d) : first(a), rest(d) {}
    
    virtual bool isAtom() const override { return false; }
    virtual bool isList() const override { return rest == nil || rest->isList(); }
//    pair *next() const { return dca<pair>(rest); }
    virtual std::string str() const override {
        return std::string("(") + first->str() + "." + rest->str() + ")";
    }

    virtual bool equals(obj* o) const override {
        pair *op = dynamic_cast<pair*>(o);
        return op && first->equals(op->first) && rest->equals(op->rest);
    }
    
};

class nilClass : public pair {
private:    
    inline static nilClass *instance = nullptr;

    explicit nilClass() : pair(this, this) {}

public:
    virtual std::string str() const override { return "'()"; }
    virtual bool isTrue() const override { return false; }

    static nilClass *getInstance() {
        if (!instance) { instance = new nilClass(); }
        return instance;
    }

    virtual bool equals(obj* o) const override { return o == this; }
};


class callable : public obj {
public:
    virtual bool isCallable() const override { return true; }
    virtual std::string str() const override {
        return isMacro ? std::string("<macro>") : std::string("<callable>");
    }
    const bool isMacro;

    explicit callable(bool m) : isMacro(m) {}

    virtual obj* call(obj* actualArgs, context* outer) const = 0;
};

class function : public callable {
    pair *formals, *body;
    context *outer;
public:
    explicit function(pair* f, pair* b, context *ctx, bool m)
        : callable(m), formals(f), body(b), outer(ctx) {}
    virtual obj* call(obj* actualArgs, context* outer) const override;
};


class builtin : public callable {
public:
    // The callback takes its argument by reference; this is a
    // compromise for performance.  We assume that the caller will
    // discard the underlying vector after the call.
    using Callback = std::function<obj*(std::vector<obj*>&, context*)>;

private:
    const Callback      code;
    const std::size_t   nargs;
    const bool          isVariadic;

public:
    explicit builtin(std::size_t na, bool isvar, bool ismacro, Callback c) :
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

static inline obj* $$(const std::string& s)  { return symbol::intern(s); }

// List construction
//static inline obj* $(void)            { return nil; }

template<typename T>
obj* $(T o)             { return new pair(_w(o), nil); }

template<typename T, typename... Objects>
obj* $(T first, Objects... rest) {
    return new pair(_w(first), $(rest...));
}

//
// basic list access
//

static inline obj* basic_first(obj *cell) {
    if (cell->isAtom() || cell == nil) { return nil; }
    return dca<pair>(cell)->first;
}

static inline obj* basic_rest(obj *cell) {
    if (cell->isAtom() || cell == nil) { return nil; }
    return dca<pair>(cell)->rest;
}

static inline obj* basic_second(obj *cell) {
    return basic_first( basic_rest(cell) );
}


static inline obj* read(std::string text) {
    std::stringstream s(text, std::ios_base::in);
    return read(s);
}


// Include prototypes for the builtins in sic_func.inc.
#define BUILTIN_FULL(name, x1,x2,x3)  \
    extern callable * const name;
#include "sic_func.inc"
#undef BUILTIN_FULL

};



