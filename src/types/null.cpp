// Copyright (c) Andrew Fischer. See LICENSE file for license terms.

namespace circa {
namespace null_t {

    std::string toString(caValue* value)
    {
        return "null";
    }
    void formatSource(StyledSource* source, Term* term)
    {
        append_phrase(source, "null", term, TK_NULL);
    }

    caValue* getField(caValue* value, const char* field)
    {
        return NULL;
    }

    void setup_type(Type* type)
    {
        type->name = name_from_string("null");
        type->toString = toString;
        type->formatSource = formatSource;
        type->getField = getField;
    }
}
}
