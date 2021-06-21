/*
Copyright 2020 Paul Zanna.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef _BACKENDS_WP4_WP4TYPE_H_
#define _BACKENDS_WP4_WP4TYPE_H_

#include "lib/algorithm.h"
#include "lib/sourceCodeBuilder.h"
#include "wp4-Object.h"
#include "ir/ir.h"

namespace WP4 {

#define UNUSED __attribute__((__unused__))

// Base class for WP4 types
class WP4Type : public WP4Object {
 protected:
    explicit WP4Type(const IR::Type* type) : type(type) {}
 public:
    const IR::Type* type;
    virtual void emit(CodeBuilder* builder) = 0;
    virtual void declare(CodeBuilder* builder, cstring id, bool asPointer) = 0;
    virtual void emitInitializer(CodeBuilder* builder) = 0;
    virtual void declareArray(CodeBuilder* /*builder*/, cstring /*id*/, unsigned /*size*/)
    { BUG("%1%: unsupported array", type); }
    template<typename T> bool is() const { return dynamic_cast<const T*>(this) != nullptr; }
    template<typename T> T *to() { return dynamic_cast<T*>(this); }
};

class IHasWidth {
 public:
    virtual ~IHasWidth() {}
    // P4 width
    virtual unsigned widthInBits() = 0;
    // Width in the target implementation.
    // Currently a multiple of 8.
    virtual unsigned implementationWidthInBits() = 0;
};

class WP4TypeFactory {
 protected:
    const P4::TypeMap* typeMap;
    explicit WP4TypeFactory(const P4::TypeMap* typeMap) :
            typeMap(typeMap) { CHECK_NULL(typeMap); }
 public:
    static WP4TypeFactory* instance;
    static void createFactory(const P4::TypeMap* typeMap)
    { WP4TypeFactory::instance = new WP4TypeFactory(typeMap); }
    virtual WP4Type* create(const IR::Type* type);
};

class WP4BoolType : public WP4Type, public IHasWidth {
 public:
    WP4BoolType() : WP4Type(IR::Type_Boolean::get()) {}
    void emit(CodeBuilder* builder) override
    { builder->append("u8"); }
    void declare(CodeBuilder* builder, cstring id, bool asPointer) override;
    void emitInitializer(CodeBuilder* builder) override
    { builder->append("0"); }
    unsigned widthInBits() override { return 1; }
    unsigned implementationWidthInBits() override { return 8; }
};

class WP4StackType : public WP4Type, public IHasWidth {
    WP4Type* elementType;
    unsigned  size;
 public:
    WP4StackType(const IR::Type_Stack* type, WP4Type* elementType) :
            WP4Type(type), elementType(elementType), size(type->getSize()) {
        CHECK_NULL(type); CHECK_NULL(elementType);
        BUG_CHECK(elementType->is<IHasWidth>(), "Unexpected element type %1%", elementType);
    }
    void emit(CodeBuilder*) override {}
    void declare(CodeBuilder* builder, cstring id, bool asPointer) override;
    void emitInitializer(CodeBuilder* builder) override;
    unsigned widthInBits() override;
    unsigned implementationWidthInBits() override;
};

class WP4ScalarType : public WP4Type, public IHasWidth {
 public:
    const unsigned width;
    const bool     isSigned;
    explicit WP4ScalarType(const IR::Type_Bits* bits) :
            WP4Type(bits), width(bits->size), isSigned(bits->isSigned) {}
    unsigned bytesRequired() const { return ROUNDUP(width, 8); }
    unsigned alignment() const;
    void emit(CodeBuilder* builder) override;
    cstring getAsString();
    cstring getSignAsString();     
    void declare(CodeBuilder* builder, cstring id, bool asPointer) override;
    void emitInitializer(CodeBuilder* builder) override
    { builder->append("0"); }
    unsigned widthInBits() override { return width; }
    unsigned implementationWidthInBits() override { return bytesRequired() * 8; }
    // True if this width is small enough to store in a machine scalar
    static bool generatesScalar(unsigned width)
    { return width <= 64; }
};

// This should not always implement IHasWidth, but it may...
class WP4TypeName : public WP4Type, public IHasWidth {
    const IR::Type_Name* type;
    WP4Type* canonical;
 public:
    WP4TypeName(const IR::Type_Name* type, WP4Type* canonical) :
            WP4Type(type), type(type), canonical(canonical) {}
    void emit(CodeBuilder* builder) override { canonical->emit(builder); }
    void declare(CodeBuilder* builder, cstring id, bool asPointer) override;
    void emitInitializer(CodeBuilder* builder) override;
    unsigned widthInBits() override;
    unsigned implementationWidthInBits() override;
    void declareArray(CodeBuilder* builder, cstring id, unsigned size) override;
};

// Also represents headers and unions
class WP4StructType : public WP4Type, public IHasWidth {
    class WP4Field {
     public:
        cstring comment;
        WP4Type* type;
        const IR::StructField* field;

        WP4Field(WP4Type* type, const IR::StructField* field, cstring comment = nullptr) :
            comment(comment), type(type), field(field) {}
    };

 public:
    cstring  kind;
    cstring  name;
    std::vector<WP4Field*>  fields;
    unsigned width;
    unsigned implWidth;

    explicit WP4StructType(const IR::Type_StructLike* strct);
    void declare(CodeBuilder* builder, cstring id, bool asPointer) override;
    void emitInitializer(CodeBuilder* builder) override;
    unsigned widthInBits() override { return width; }
    unsigned implementationWidthInBits() override { return implWidth; }
    void emit(CodeBuilder* builder) override;
    void declareArray(CodeBuilder* builder, cstring id, unsigned size) override;
};

class WP4EnumType : public WP4Type, public WP4::IHasWidth {
 public:
    explicit WP4EnumType(const IR::Type_Enum* type) : WP4Type(type) {}
    void emit(CodeBuilder* builder) override;
    void declare(CodeBuilder* builder, cstring id, bool asPointer) override;
    void emitInitializer(CodeBuilder* builder) override
    { builder->append("0"); }
    unsigned widthInBits() override { return 32; }
    unsigned implementationWidthInBits() override { return 32; }
    const IR::Type_Enum* getType() const { return type->to<IR::Type_Enum>(); }
};

}  // namespace WP4

#endif /* _BACKENDS_WP4_WP4TYPE_H_ */
