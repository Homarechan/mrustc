/*
 */
#include "ast.hpp"
#include "../types.hpp"
#include <iostream>
#include "../parse/parseerror.hpp"
#include <algorithm>

namespace AST {

ExternCrate ExternCrate_std();


SERIALISE_TYPE(MetaItem::, "AST_MetaItem", {
    s << m_name;
    s << m_str_val;
    s << m_items;
})

::std::ostream& operator<<(::std::ostream& os, const Pattern& pat)
{
    switch(pat.m_class)
    {
    case Pattern::ANY:
        os << "Pattern(TagWildcard, '" << pat.m_binding << "' @ _)";
        break;
    case Pattern::MAYBE_BIND:
        os << "Pattern(TagMaybeBind, '" << pat.m_binding << "')";
        break;
    case Pattern::VALUE:
        //os << "Pattern(TagValue, " << *pat.m_node << ")";
        os << "Pattern(TagValue, '" << pat.m_binding << "' @ TODO:ExprNode)";
        break;
    case Pattern::TUPLE:
        os << "Pattern(TagTuple, '" << pat.m_binding << "' @ [" << pat.m_sub_patterns << "])";
        break;
    case Pattern::TUPLE_STRUCT:
        os << "Pattern(TagEnumVariant, '" << pat.m_binding << "' @ " << pat.m_path << ", [" << pat.m_sub_patterns << "])";
        break;
    }
    return os;
}


Impl::Impl(TypeRef impl_type, TypeRef trait_type)
{
}
void Impl::add_function(bool is_public, ::std::string name, Function fcn)
{
}

Crate::Crate():
    m_root_module(*this, ""),
    m_load_std(true)
{
}
void Crate::iterate_functions(fcn_visitor_t* visitor)
{
    m_root_module.iterate_functions(visitor, *this);
}
Module& Crate::get_root_module(const ::std::string& name) {
    return const_cast<Module&>( const_cast<const Crate*>(this)->get_root_module(name) );
}
const Module& Crate::get_root_module(const ::std::string& name) const {
    if( name == "" )
        return m_root_module;
    auto it = m_extern_crates.find(name);
    if( it != m_extern_crates.end() )
        return it->second.root_module();
    throw ParseError::Generic("crate name unknown");
}
void Crate::load_extern_crate(::std::string name)
{
    if( name == "std" )
    {
        // HACK! Load std using a hackjob (included within the compiler)
        m_extern_crates.insert( make_pair( ::std::move(name), ExternCrate_std() ) );
    }
    else
    {
        throw ParseError::Todo("'extern crate' (not hackjob std)");
    }
}
SERIALISE_TYPE(Crate::, "AST_Crate", {
    s << m_load_std;
    s << m_extern_crates;
    s << m_root_module;
})

ExternCrate::ExternCrate()
{
}

ExternCrate::ExternCrate(const char *path)
{
    throw ParseError::Todo("Load extern crate from a file");
}
SERIALISE_TYPE(ExternCrate::, "AST_ExternCrate", {
})

ExternCrate ExternCrate_std()
{
    ExternCrate crate;
    
    Module& std_mod = crate.root_module();
    
    // TODO: Add modules
    Module  option(crate.crate(), "option");
    option.add_enum(true, "Option", Enum(
        {
            TypeParam(false, "T"),
        },
        {
            StructItem("None", TypeRef()),
            StructItem("Some", TypeRef(TypeRef::TagArg(), "T")),
        }
        ));
    std_mod.add_submod(true, ::std::move(option));
    
    Module  prelude(crate.crate(), "prelude");
    // Re-exports
    #define USE(mod, name, ...)    do{ Path p({__VA_ARGS__}); mod.add_alias(true, ::std::move(p), name); } while(0)
    USE(prelude, "Option",  PathNode("option", {}), PathNode("Option",{}) );
    USE(prelude, "Some",  PathNode("option", {}), PathNode("Option",{}), PathNode("Some",{}) );
    USE(prelude, "None",  PathNode("option", {}), PathNode("Option",{}), PathNode("None",{}) );
    std_mod.add_submod(true, prelude);
    
    return crate;
}

SERIALISE_TYPE(Module::, "AST_Module", {
    s << m_name;
    s << m_attrs;
    s << m_extern_crates;
    s << m_submods;
    s << m_enums;
    s << m_structs;
    s << m_functions;
})
void Module::add_ext_crate(::std::string ext_name, ::std::string int_name)
{
    DEBUG("add_ext_crate(\"" << ext_name << "\" as " << int_name << ")");
    m_crate.load_extern_crate(ext_name);
    
    m_extern_crates.push_back( Item< ::std::string>( ::std::move(int_name), ::std::move(ext_name), false ) );
}
void Module::add_constant(bool is_public, ::std::string name, TypeRef type, Expr val)
{
    ::std::cout << "add_constant()" << ::std::endl;
}
void Module::add_global(bool is_public, bool is_mut, ::std::string name, TypeRef type, Expr val)
{
    ::std::cout << "add_global()" << ::std::endl;
}
void Module::add_struct(bool is_public, ::std::string name, TypeParams params, ::std::vector<StructItem> items)
{
}
void Module::add_impl(Impl impl)
{
}
void Module::iterate_functions(fcn_visitor_t *visitor, const Crate& crate)
{
    for( auto fcn_item : this->m_functions )
    {
        visitor(crate, *this, fcn_item.data);
    }
}

::Serialiser& operator<<(::Serialiser& s, Function::Class fc)
{
    switch(fc)
    {
    case Function::CLASS_UNBOUND: s << "UNBOUND"; break;
    case Function::CLASS_REFMETHOD: s << "REFMETHOD"; break;
    case Function::CLASS_MUTMETHOD: s << "MUTMETHOD"; break;
    case Function::CLASS_VALMETHOD: s << "VALMETHOD"; break;
    }
    return s;
}
SERIALISE_TYPE(Function::, "AST_Function", {
    s << m_fcn_class;
    s << m_generic_params;
    s << m_rettype;
    s << m_args;
    //s << m_code;
})

SERIALISE_TYPE(Enum::, "AST_Enum", {
    s << m_params;
    s << m_variants;
})

SERIALISE_TYPE(Struct::, "AST_Struct", {
    s << m_params;
    s << m_fields;
})

TypeParam::TypeParam(bool is_lifetime, ::std::string name)
{

}
void TypeParam::addLifetimeBound(::std::string name)
{

}
void TypeParam::addTypeBound(TypeRef type)
{

}
SERIALISE_TYPE(TypeParam::, "AST_TypeParam", {
    // TODO: TypeParam
})

}
