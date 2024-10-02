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
from time import sleep
from typing import List

from binaryninja.types import BaseStructure
from binaryninja import BinaryView, BaseStructure, NamedTypeReferenceClass, StructureVariant, NamedTypeReferenceType, \
    StructureBuilder, PointerType, PluginCommandContext, PluginCommand

import sys
if len(sys.argv) != 2:
    print("Usage: python class_dump.py <path_to_file>")
    sys.exit(1)
file_path = sys.argv[1]

print("Loading binary...")
view = BinaryView.load(file_path)
cxt = PluginCommandContext(view)
print("Finding RTTI...")
PluginCommand.get_valid_list(cxt)['MSVC\\Find RTTI'].execute(cxt)

# Wait for metadata to be valid
while True:
    try:
        if view.query_metadata("msvc"):
            break
    except KeyError as e:
        sleep(1)
data = view.query_metadata("msvc")

class ClassInfo:
    def __init__(self, class_name: str, base_classes: dict[int, str]):
        self.class_name = class_name
        self.base_classes = base_classes
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
    print(class_info)
