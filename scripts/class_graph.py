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


from PySide6.QtWidgets import QApplication, QDialog, QVBoxLayout
from PySide6.QtGui import QFont
from PySide6.QtCore import QSize
# FYI we do not have this in binja
from PySide6.QtCharts import QChart, QChartView, QLineSeries

data = {
    'classes': {
        '5368823328': {'className': 'Animal'},
        '5368823464': {'className': 'Flying'},
        '5368823600': {'className': 'Bird'},
        '5368823808': {'baseClassName': 'Flying', 'className': 'Bird', 'classOffset': 16},
        '5368823856': {'className': 'type_info'}
    }
}

class GraphDialog(QDialog):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Class Hierarchy Graph")
        self.resize(800, 600)

        layout = QVBoxLayout(self)

        self.chart = QChart()
        self.chart_view = QChartView(self.chart)

        self.series = QLineSeries()
        self.add_data_to_series()

        self.chart.addSeries(self.series)
        self.chart.createDefaultAxes()

        layout.addWidget(self.chart_view)

    def add_data_to_series(self):
        # Dummy implementation of graph based only on the given data; here we'll just display one line from (0,0) to (1,1)
        self.series.append(0, 0)
        self.series.append(1, 1)

        # Add nodes and connections based on the data provided
        nodes = data['classes']
        positions = {}
        x, y = 0, 0

        for node_id, node_info in nodes.items():
            class_name = node_info.get('className', 'Unknown')
            base_class_name = node_info.get('baseClassName', None)
            class_offset = node_info.get('classOffset', 0)

            positions[class_name] = (x, y + class_offset)
            x += 1

            if base_class_name:
                base_position = positions.get(base_class_name, (0, 0))
                self.series.append(*base_position)
                self.series.append(x, y)

            y += 10

def create_window():
    dialog = GraphDialog()
    dialog.exec_()


execute_on_main_thread(create_window)