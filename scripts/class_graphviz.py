from time import sleep
import graphviz
import sys
from binaryninja import BinaryView, PluginCommandContext, PluginCommand

# Whether to open the png after it is created.
AUTO_OPEN = True

if len(sys.argv) != 3:
    print("Usage: python class_graphviz.py <path_to_file> <output_path>")
    sys.exit(1)
file_path = sys.argv[1]
out_path = sys.argv[1]

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
        sleep(0.2)

print("Creating graph...")
data = view.query_metadata("msvc")

def create_graph(data):
    dot = graphviz.Digraph()
    classes = data.get('classes', {})
    for class_info in classes.values():
        class_name = class_info.get('className')
        base_class_name = class_info.get('baseClassName', None)
        dot.node(class_name)
        if base_class_name:
            dot.edge(base_class_name, class_name)
    return dot

# data = {
#         'classes': {
#             '5368823328': {'className': 'Animal'},
#             '5368823464': {'className': 'Flying'},
#             '5368823600': {'className': 'Bird'},
#             '5368823808': {'baseClassName': 'Flying', 'className': 'Bird', 'classOffset': 16},
#             '5368823856': {'className': 'type_info'}
#         }
# }
dot = create_graph(data)
dot.render(out_path, view=AUTO_OPEN, format='png')
