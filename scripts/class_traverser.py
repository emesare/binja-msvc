from PySide6.QtWidgets import (QVBoxLayout, QTreeWidget,
                               QTreeWidgetItem, QDialog, QLabel, QLineEdit)

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
data = bv.query_metadata("msvc")


class MainDialog(QDialog):
    def __init__(self):
        super().__init__()

        self.setWindowTitle("Class Traverser")
        self.setGeometry(100, 100, 600, 400)

        # Create the main layout
        main_layout = QVBoxLayout()

        # Create the filter input
        self.filter_input = QLineEdit()
        self.filter_input.setPlaceholderText("Filter classes")
        self.filter_input.textChanged.connect(self.filter_tree)
        main_layout.addWidget(self.filter_input)

        # Create the tree widget
        self.tree_widget = QTreeWidget()
        self.tree_widget.setHeaderLabels(["Address", "Class Name", "Base Class Name", "Class Offset"])

        # Populate the tree widget using the given data
        self.populate_tree(data["classes"])

        # Add the tree widget and buttons to the layout
        main_layout.addWidget(self.tree_widget)

        # Add a label to show the number of classes
        num_classes_label = QLabel(f"Number of classes: {len(data['classes'])}")
        main_layout.addWidget(num_classes_label)

        # Set the main layout for the dialog
        self.setLayout(main_layout)

    def filter_tree(self, text):
        for i in range(self.tree_widget.topLevelItemCount()):
            item = self.tree_widget.topLevelItem(i)
            self.filter_tree_recursively(item, text.lower())

    def filter_tree_recursively(self, item, filter_text):
        match = filter_text in item.text(1).lower() or filter_text in item.text(2).lower()
        item.setHidden(not match)
        for j in range(item.childCount()):
            child = item.child(j)
            self.filter_tree_recursively(child, filter_text)

    def populate_tree(self, classes):
        sorted_classes = dict(sorted(classes.items(), key=lambda item: item[1].get('className', '')))
        # Connect the double-click signal to a handler function
        self.tree_widget.itemDoubleClicked.connect(self.on_item_double_clicked)
        for class_addr, class_info in sorted_classes.items():
            tree_item = QTreeWidgetItem()
            tree_item.setText(0, hex(int(class_addr)))
            tree_item.setText(1, class_info.get("className", ""))
            tree_item.setText(2, class_info.get("baseClassName", ""))
            tree_item.setText(3, str(class_info.get("classOffset", "")))
            self.tree_widget.addTopLevelItem(tree_item)

    def on_item_double_clicked(self, item, column):
        bv.navigate(bv.view, int(item.text(0), 0))


def create_window():
    dialog = MainDialog()
    dialog.exec_()


execute_on_main_thread(create_window)
