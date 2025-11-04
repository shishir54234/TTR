#include "functionfactory.hh"

template <typename DerivedType, typename BaseType>
unique_ptr<DerivedType> dynamic_pointer_cast(std::unique_ptr<BaseType>& basePtr) {
    if (DerivedType* derivedRawPtr = dynamic_cast<DerivedType*>(basePtr.get())) {
        return unique_ptr<DerivedType>(static_cast<DerivedType*>(basePtr.release()));
    }
    return nullptr;
}
