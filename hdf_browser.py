import sys
import numpy as np
import h5py
import json
import csv
import datetime
from PySide6.QtWidgets import (
    QApplication, QMainWindow, QFileDialog,
    QTreeWidget, QTreeWidgetItem, QTextEdit, QVBoxLayout, QWidget, QPushButton,
    QHBoxLayout, QMessageBox, QInputDialog, QDialog, QLabel, QCalendarWidget, QTimeEdit,
    QListWidget, QListWidgetItem, QDialogButtonBox, QCheckBox
)
from PySide6.QtGui import QAction
from PySide6.QtCore import QDateTime, Qt

import matplotlib.pyplot as plt
import matplotlib.dates as mdates
from matplotlib.backends.backend_qtagg import NavigationToolbar2QT as NavigationToolbar
from matplotlib.backends.backend_qtagg import FigureCanvasQTAgg as FigureCanvas


class FechaHoraDialog(QDialog):
    def __init__(self, parent=None, titulo="Seleccione fecha y hora"):
        super().__init__(parent)
        self.setWindowTitle(titulo)
        self.setModal(True)
        self.resize(10, 10)

        layout = QVBoxLayout()

        self.calendar = QCalendarWidget()
        self.calendar.setGridVisible(True)
        layout.addWidget(self.calendar)

        hlayout = QHBoxLayout()
        hlayout.addWidget(QLabel("Hora:"))
        self.time_edit = QTimeEdit()
        self.time_edit.setDisplayFormat("HH:mm:ss")
        self.time_edit.setTime(self.time_edit.time().currentTime())
        hlayout.addWidget(self.time_edit)
        layout.addLayout(hlayout)

        btn_layout = QHBoxLayout()
        self.ok_btn = QPushButton("Aceptar")
        self.ok_btn.clicked.connect(self.accept)
        btn_layout.addWidget(self.ok_btn)
        self.cancel_btn = QPushButton("Cancelar")
        self.cancel_btn.clicked.connect(self.reject)
        btn_layout.addWidget(self.cancel_btn)

        layout.addLayout(btn_layout)
        self.setLayout(layout)

    def get_datetime(self):
        date = self.calendar.selectedDate()
        time = self.time_edit.time()
        dt = QDateTime(date, time, Qt.LocalTime)
        return dt.toPython()


class HDF5Browser(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Mini HDF5 Browser")
        self.setMinimumSize(100, 100)
        self.hdf = None
        self.current_dataset_path = None

        self.tree = QTreeWidget()
        self.tree.setHeaderLabels(["Estructura HDF5"])
        self.tree.itemClicked.connect(self.on_item_clicked)

        self.text_view = QTextEdit()
        self.text_view.setReadOnly(True)

        self.open_btn = QPushButton("Abrir archivo HDF5")
        self.open_btn.clicked.connect(self.load_file)

        self.save_btn = QPushButton("Guardar dataset como CSV")
        self.save_btn.clicked.connect(self.save_dataset)
        self.save_btn.setEnabled(False)

        self.attr_btn = QPushButton("Editar atributos")
        self.attr_btn.clicked.connect(self.edit_attributes)
        self.attr_btn.setEnabled(False)

        self.export_btn = QPushButton("Exportar estructura JSON")
        self.export_btn.clicked.connect(self.export_structure)
        self.export_btn.setEnabled(False)

        self.plot_btn = QPushButton("Graficar dataset")
        self.plot_btn.clicked.connect(self.plot_dataset)
        self.plot_btn.setEnabled(False)

        top_layout = QHBoxLayout()
        top_layout.addWidget(self.open_btn)
        top_layout.addWidget(self.save_btn)
        top_layout.addWidget(self.attr_btn)
        #top_layout.addWidget(self.export_btn)
        top_layout.addWidget(self.plot_btn)

        layout = QVBoxLayout()
        layout.addLayout(top_layout)
        layout.addWidget(self.tree)
        layout.addWidget(self.text_view)

        container = QWidget()
        container.setLayout(layout)
        self.setCentralWidget(container)

    def load_file(self):
        path, _ = QFileDialog.getOpenFileName(self, "Selecciona archivo HDF5", "", "Archivos HDF5 (*.h5 *.hdf5)")
        if path:
            if self.hdf:
                self.hdf.close()
            self.tree.clear()
            self.hdf = h5py.File(path, "r")
            self.build_tree(self.hdf, self.tree.invisibleRootItem())
            self.setWindowTitle(f"Mini HDF5 Browser - {path}")
            self.export_btn.setEnabled(True)

    def build_tree(self, obj, parent_item):
        for key in obj:
            item = QTreeWidgetItem([key])
            parent_item.addChild(item)
            node = obj[key]
            if isinstance(node, h5py.Group):
                self.build_tree(node, item)
            elif isinstance(node, h5py.Dataset):
                item.setToolTip(0, f"Dataset: shape={node.shape}, dtype={node.dtype}")

    def on_item_clicked(self, item, _):
        full_path = self.get_full_path(item)
        try:
            node = self.hdf[full_path]
            self.current_dataset_path = full_path if isinstance(node, h5py.Dataset) else None
            self.save_btn.setEnabled(isinstance(node, h5py.Dataset))
            self.attr_btn.setEnabled(True)
            self.plot_btn.setEnabled(isinstance(node, h5py.Dataset))

            info = f"Ruta: {full_path}\n"
            if isinstance(node, h5py.Dataset):
                info += f"\nDataset: shape={node.shape}, dtype={node.dtype}\n"
                data = node[()]
                info += f"\nContenido:\n{data}\n"
            elif isinstance(node, h5py.Group):
                info += f"\nGrupo\n"

            if node.attrs:
                info += "\nAtributos:\n"
                for k, v in node.attrs.items():
                    info += f"  {k}: {v}\n"

            self.text_view.setText(info)
        except Exception as e:
            self.text_view.setText(f"Error: {str(e)}")

    def get_full_path(self, item):
        names = []
        while item:
            names.insert(0, item.text(0))
            item = item.parent()
        return "/" + "/".join(names)

    def save_dataset(self):
        item = self.tree.currentItem()
        if not item:
            return
        path = self.get_full_path(item)
        try:
            data = self.hdf[path][()]
            if data.ndim > 2:
                QMessageBox.warning(self, "Guardar como CSV", "Solo se pueden guardar datasets de 1 o 2 dimensiones.")
                return
            csv_path, _ = QFileDialog.getSaveFileName(self, "Guardar como CSV", "", "CSV Files (*.csv)")
            if csv_path:
                with open(csv_path, "w", newline="") as f:
                    writer = csv.writer(f)
                    if data.ndim == 1 and data.dtype.names:
                        writer.writerow(data.dtype.names)
                        for row in data:
                            writer.writerow([row[name] for name in data.dtype.names])
                    elif data.ndim == 1:
                        writer.writerow(data)
                    else:
                        writer.writerows(data)
                QMessageBox.information(self, "Éxito", "Dataset guardado como CSV correctamente.")
        except Exception as e:
            QMessageBox.critical(self, "Error", str(e))

    def edit_attributes(self):
        item = self.tree.currentItem()
        if not item:
            return
        path = self.get_full_path(item)
        try:
            node = self.hdf[path]
            for key in node.attrs:
                val, ok = QInputDialog.getText(self, f"Editar atributo: {key}", "Nuevo valor:", text=str(node.attrs[key]))
                if ok:
                    node.attrs[key] = val
            self.on_item_clicked(item, None)
        except Exception as e:
            QMessageBox.critical(self, "Error", str(e))

    def export_structure(self):
        if not self.hdf:
            return

        def build_dict(obj):
            result = {}
            for key in obj:
                if isinstance(obj[key], h5py.Group):
                    result[key] = build_dict(obj[key])
                else:
                    result[key] = {
                        "shape": obj[key].shape,
                        "dtype": str(obj[key].dtype)
                    }
            return result

        structure = build_dict(self.hdf)
        json_path, _ = QFileDialog.getSaveFileName(self, "Guardar estructura como JSON", "", "JSON (*.json)")
        if json_path:
            with open(json_path, "w") as f:
                json.dump(structure, f, indent=2)
            QMessageBox.information(self, "Éxito", "Estructura exportada a JSON.")

    def plot_dataset(self):
        if not self.current_dataset_path:
            QMessageBox.warning(self, "Graficar dataset", "Selecciona primero un dataset para graficar.")
            return

        try:
            data = self.hdf[self.current_dataset_path][()]
            if data.ndim != 1 or not data.dtype.names:
                QMessageBox.warning(self, "Graficar dataset", "Solo se soportan datasets estructurados unidimensionales.")
                return

            campos = list(data.dtype.names)
            if "Timestamp" not in campos:
                QMessageBox.warning(self, "Graficar dataset", "El dataset debe contener un campo 'Timestamp'.")
                return

            dialog_inicio = FechaHoraDialog(self, "Fecha y hora de inicio")
            if dialog_inicio.exec() == QDialog.Accepted:
                t_inicio = dialog_inicio.get_datetime()
            else:
                return

            dialog_fin = FechaHoraDialog(self, "Fecha y hora de término")
            if dialog_fin.exec() == QDialog.Accepted:
                t_fin = dialog_fin.get_datetime()
            else:
                return

            if not t_inicio or not t_fin or t_inicio >= t_fin:
                QMessageBox.warning(self, "Rango inválido", "Verifica que las fechas sean válidas y que el inicio sea menor al fin.")
                return

            timestamps = data["Timestamp"].astype(float)
            mask = (timestamps >= t_inicio.timestamp()) & (timestamps <= t_fin.timestamp())
            if not np.any(mask):
                QMessageBox.warning(self, "Sin datos", "No hay datos en el rango seleccionado.")
                return

            x = [datetime.datetime.fromtimestamp(ts) for ts in timestamps[mask]]
            campos_y = [c for c in campos if c != "Timestamp"]
            colores = ['tab:red', 'tab:blue']

            #plt.style.use('dark_background')
            fig, ax1 = plt.subplots()
            canvas = FigureCanvas(fig)
            toolbar = NavigationToolbar(canvas)

            datos = {var: data[var][mask] for var in campos_y}
            ax2 = ax1.twinx()
            lineas = {}

            def actualizar_plot():
                seleccionadas = [var for var, cb in checkboxes.items() if cb.isChecked()]
                if len(seleccionadas) > 2:
                    QMessageBox.warning(widget, "Límite de variables", "Solo se pueden graficar hasta 2 variables a la vez.")
                    for cb in checkboxes.values():
                        cb.blockSignals(True)
                    for var in seleccionadas[2:]:
                        checkboxes[var].setChecked(False)
                    for cb in checkboxes.values():
                        cb.blockSignals(False)
                    seleccionadas = seleccionadas[:2]

                ax1.cla()
                ax2.cla()
                lineas.clear()
                
                ax1.grid(True, which='major', linestyle='-', linewidth=0.5, color='gray')
                ax1.minorticks_on()
                ax1.grid(True, which='minor', linestyle=':', linewidth=0.3, color='gray')
                ax1.xaxis.set_major_formatter(mdates.DateFormatter("%Y-%m-%d %H:%M:%S"))
                fig.autofmt_xdate()

                if len(seleccionadas) == 0:
                    # No mostrar ticks ni labels en vertical
                    ax1.set_yticks([])
                    ax1.set_ylabel("")
                    ax1.tick_params(axis='y', length=0, labelcolor='none')

                    ax2.set_yticks([])
                    ax2.set_ylabel("")
                    ax2.tick_params(axis='y', length=0, labelcolor='none')

                elif len(seleccionadas) == 1:
                    y1 = datos[seleccionadas[0]]
                    ax1.plot(x, y1, color=colores[0], label=seleccionadas[0])
                    ax1.set_ylabel(seleccionadas[0], color=colores[0])
                    ax1.tick_params(axis='y', labelcolor=colores[0], length=5)

                    # Eje derecho sin ticks ni etiquetas
                    ax2.set_yticks([])
                    ax2.set_ylabel("")
                    ax2.tick_params(axis='y', length=0, labelcolor='none')

                else:  # dos variables
                    y1 = datos[seleccionadas[0]]
                    ax1.plot(x, y1, color=colores[0], label=seleccionadas[0])
                    ax1.set_ylabel(seleccionadas[0], color=colores[0])
                    ax1.tick_params(axis='y', labelcolor=colores[0], length=5)

                    y2 = datos[seleccionadas[1]]
                    ax2.plot(x, y2, color=colores[1], label=seleccionadas[1])
                    # Etiqueta Y derecha al lado derecho
                    ax2.set_ylabel(seleccionadas[1], color=colores[1], rotation=270)
                    ax2.yaxis.set_label_coords(1.06, 0.5)

                    ax2.tick_params(axis='y', labelcolor=colores[1], length=5)

                canvas.draw()

            checkbox_layout = QHBoxLayout()
            checkboxes = {}
            from PySide6.QtWidgets import QCheckBox
            for var in campos_y:
                cb = QCheckBox(var)
                cb.stateChanged.connect(actualizar_plot)
                checkboxes[var] = cb
                checkbox_layout.addWidget(cb)

            export_btn = QPushButton("Exportar CSV")

            def export_csv():
                if not lineas:
                    QMessageBox.warning(widget, "Sin datos", "No hay datos seleccionados para exportar.")
                    return
                path, _ = QFileDialog.getSaveFileName(widget, "Guardar CSV", "", "CSV (*.csv)")
                if path:
                    try:
                        with open(path, "w", newline="") as f:
                            writer = csv.writer(f)
                            encabezado = ["Timestamp"] + list(lineas.keys())
                            writer.writerow(encabezado)
                            for i in range(len(x)):
                                fila = [x[i].isoformat()] + [lineas[var][1][i] for var in lineas]
                                writer.writerow(fila)
                        QMessageBox.information(widget, "Éxito", "Datos exportados correctamente.")
                    except Exception as ex:
                        QMessageBox.critical(widget, "Error", str(ex))

            export_btn.clicked.connect(export_csv)

            layout = QVBoxLayout()
            layout.addWidget(toolbar)
            layout.addWidget(canvas)
            layout.addLayout(checkbox_layout)
            layout.addWidget(export_btn)

            widget = QWidget()
            widget.setLayout(layout)

            plot_window = QMainWindow(self)
            plot_window.setWindowTitle("TwisTorr84FS Plot")
            plot_window.setCentralWidget(widget)
            plot_window.resize(1000, 700)
            plot_window.show()

        except Exception as e:
            QMessageBox.critical(self, "Error al graficar", str(e))


if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = HDF5Browser()
    window.show()
    sys.exit(app.exec())
