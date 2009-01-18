// Copyright 2008 Paul Hodge

#include "common_headers.h"

#include "branch.h"
#include "builtins.h"
#include "introspection.h"
#include "parser.h"
#include "runtime.h"
#include "ref_map.h"
#include "term.h"
#include "type.h"
#include "values.h"
#include "wrappers.h"

namespace circa {

int DEBUG_CURRENTLY_INSIDE_BRANCH_DESTRUCTOR = 0;

Branch::~Branch()
{
    DEBUG_CURRENTLY_INSIDE_BRANCH_DESTRUCTOR++;

    // Create a map where all our terms go to NULL
    ReferenceMap deleteMap;
    
    std::vector<Term*>::iterator it;
    for (it = _terms.begin(); it != _terms.end(); ++it) {
        if (*it != NULL)
            deleteMap[*it] = NULL;
    }

    this->remapPointers(deleteMap);

    // dealloc_value on all non-types
    for (unsigned int i = 0; i < _terms.size(); i++)
    {
        Term *term = _terms[i];

        if (term == NULL)
            continue;

        assert_good_pointer(term);

        if (term->type != TYPE_TYPE)
            dealloc_value(term);
    }

    // delete everybody
    for (unsigned int i = 0; i < _terms.size(); i++)
    {
        Term *term = _terms[i];
        if (term == NULL)
            continue;

        assert_good_pointer(term);

        dealloc_value(term);
        term->owningBranch = NULL;
        delete term;
    }

    DEBUG_CURRENTLY_INSIDE_BRANCH_DESTRUCTOR--;
}

void Branch::append(Term* term)
{
    assert_good_pointer(term);
    assert(term->owningBranch == NULL);
    term->owningBranch = this;
    _terms.push_back(term);
}

void Branch::bindName(Term* term, std::string name)
{
    names.bind(term, name);

#if 0
    // enable this code when subroutine inputs can work without having
    // multiple name bindings.
    if (term->name != "") {
        throw std::runtime_error(std::string("term already has name: ")+term->name);
    }
#endif

    term->name = name;
}

Term* Branch::findNamed(std::string const& name) const
{
    if (containsName(name))
        return getNamed(name);

    if (outerScope != NULL) {
        return outerScope->findNamed(name);
    }

    return get_global(name);
}

void Branch::remapPointers(ReferenceMap const& map)
{
    names.remapPointers(map);

    std::vector<Term*>::iterator it;
    for (it = _terms.begin(); it != _terms.end(); ++it) {
        if (*it != NULL)
            remap_pointers(*it, map);
    }
}

void Branch::visitPointers(PointerVisitor& visitor)
{
    struct VisitPointerIfOutsideBranch : PointerVisitor
    {
        Branch *branch;
        PointerVisitor &visitor;

        VisitPointerIfOutsideBranch(Branch *_branch, PointerVisitor &_visitor)
            : branch(_branch), visitor(_visitor) {}

        virtual void visitPointer(Term* term)
        {
            if (term == NULL)
                return;
            if (term->owningBranch != branch)
                visitor.visitPointer(term);
        }
    };

    VisitPointerIfOutsideBranch myVisitor(this, visitor);

    std::vector<Term*>::iterator it;
    for (it = _terms.begin(); it != _terms.end(); ++it) {
        if (*it == NULL)
            continue;

        // Type& type = as_type((*it)->type);

        visit_pointers(*it, myVisitor);
    }
}

void
Branch::clear()
{
    _terms.clear();
    names.clear();
}

void Branch::eval()
{
    evaluate_branch(*this);
}

Term*
Branch::eval(std::string const& statement)
{
    return eval_statement(this, statement);
}

Term*
Branch::compile(std::string const& statement)
{
    return compile_statement(this, statement);
}

Branch& Branch::startBranch(std::string const& name)
{
    Term* result = create_value(this, BRANCH_TYPE, name);
    as_branch(result).outerScope = this;
    return as_branch(result);
}

void
Branch::hosted_remap_pointers(Term* caller, ReferenceMap const& map)
{
    as_branch(caller).remapPointers(map);
}

void
Branch::hosted_visit_pointers(Term* caller, PointerVisitor& visitor)
{
    as_branch(caller).visitPointers(visitor);
}

void
Branch::hosted_update_owner(Term* caller)
{
    as_branch(caller).owningTerm = caller;
}

Branch& as_branch(Term* term)
{
    assert_type(term, BRANCH_TYPE);
    assert(term->value != NULL);
    return *((Branch*) term->value);
}

std::string get_name_for_attribute(std::string attribute)
{
    return "#attr:" + attribute;
}

void duplicate_branch(Branch& source, Branch& dest)
{
    ReferenceMap newTermMap;

    // Duplicate every term
    for (int index=0; index < source.numTerms(); index++) {
        Term* source_term = source.get(index);

        Term* dest_term = create_term(&dest, source_term->function, source_term->inputs);
        newTermMap[source_term] = dest_term;

        duplicate_value(source_term, dest_term);
    }

    // Remap terms
    for (int index=0; index < dest.numTerms(); index++) {
        Term* term = dest.get(index);
        remap_pointers(term, newTermMap);
    }

    // Copy names
    TermNamespace::StringToTermMap::iterator it;
    for (it = source.names.begin(); it != source.names.end(); ++it) {
        std::string name = it->first;
        Term* original_term = it->second;
        dest.bindName(newTermMap.getRemapped(original_term), name);
    }
}

void migrate_branch(Branch& original, Branch& replacement)
{
    // This function is a work in progress

    // For now, just look for named terms in 'original', and replace
    // values with values from 'replacement'
    for (int termIndex=0; termIndex < original.numTerms(); termIndex++) {
        Term* term = original[termIndex];
        if (term->name == "")
            continue;

        if (!replacement.containsName(term->name))
            // todo: print a warning here?
            continue;

        Term* replaceTerm = replacement[term->name];

        // Special behavior for branches
        if (term->state != NULL && term->state->type == BRANCH_TYPE) {
            migrate_branch(as_branch(term->state), as_branch(replaceTerm->state));
        } else if (is_value(term)) {
            assign_value(replaceTerm, term);
        }
    }
}

void evaluate_file(Branch& branch, std::string const& filename)
{
    std::string fileContents = read_text_file(filename);

    token_stream::TokenStream tokens(fileContents);
    ast::StatementList *statementList = parser::statementList(tokens);

    CompilationContext context;
    context.pushScope(&branch, NULL);
    statementList->createTerms(context);
    context.popScope();

    delete statementList;

    string_value(branch, filename, get_name_for_attribute("source-file"));
}

void reload_branch_from_file(Branch& branch)
{
    std::string filename = as_string(branch[get_name_for_attribute("source-file")]);

    Branch replacement;

    evaluate_file(replacement, filename);

    migrate_branch(branch, replacement);
}

} // namespace circa
