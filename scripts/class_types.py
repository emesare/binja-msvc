# data looks like:
# {
#     'classes': {
#         '5368823328': {'className': 'Animal'},
#         '5368823464': {'className': 'Flying'},
#         '5368823600': {'className': 'Bird'},
#         '5368823808': {'baseClassName': 'Flying', 'className': 'Bird', 'classOffset': 16},
#         '5368823856': {'className': 'type_info'}
#     }
# }
from typing import List

from binaryninja.types import BaseStructure
from binaryninja import BinaryView, BaseStructure, NamedTypeReferenceClass, StructureVariant, NamedTypeReferenceType, \
    StructureBuilder, PointerType

view: BinaryView = bv
data = view.query_metadata("msvc")


class ClassInfo:
    def __init__(self, class_name: str, base_classes: dict[int, str]):
        self.class_name = class_name
        self.base_classes = base_classes

    def create_type(self, view: BinaryView):
        # Check if the type name already exists in the view
        if self.class_name in view.type:
            return
        # TODO: Some classes are skipped in creation...
        # TODO: I think they are ones that did not get a non base class node?
        # Create a new Binary Ninja structure type for the class.
        struct_builder = StructureBuilder.create(None, StructureVariant.ClassStructureType)
        should_add_vtable = True
        # Add base classes to the structure with appropriate offsets.
        base_structs: List[BaseStructure] = []
        for offset, base_class_name in class_info.base_classes.items():
            if offset == 0:
                should_add_vtable = False  # Vtable at 0 is not our class vtable...
            base_struct_ntr = NamedTypeReferenceType.create(NamedTypeReferenceClass.ClassNamedTypeClass,
                                                            None, base_class_name)
            base_structs.append(BaseStructure(base_struct_ntr, offset))
        struct_builder.base_structures = base_structs
        if should_add_vtable:
            vtable_ntr = NamedTypeReferenceType.create(NamedTypeReferenceClass.StructNamedTypeClass, None,
                                                       self.class_name + "::VTable")
            vtable_ntr_ptr = PointerType.create(view.arch, vtable_ntr, True)
            struct_builder.insert(0, vtable_ntr_ptr, "vtable", False)
        # Finally create the type.
        view.define_type("msvc_" + self.class_name, self.class_name, struct_builder)

    def __repr__(self):
        return f"ClassInfo(class_name={self.class_name}, base_classes={self.base_classes})"


classes = {}
for class_addr, class_data in data['classes'].items():
    class_name = class_data.get('className')
    base_class_name = class_data.get('baseClassName', None)
    base_class_offset = class_data.get('classOffset', 0)
    if class_name not in classes:
        classes[class_name] = ClassInfo(class_name=class_name, base_classes={})
    if base_class_name:
        classes[class_name].base_classes[base_class_offset] = base_class_name

for class_info in classes.values():
    class_info.create_type(view)
