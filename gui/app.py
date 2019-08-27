import sys
from PyQt5.QtCore import Qt, QTimer
from PyQt5.QtWidgets import (
    QApplication, QWidget, QPushButton, QDesktopWidget, 
    QMainWindow, QAction, QVBoxLayout, QLCDNumber, QDockWidget, QInputDialog, QFileDialog,
    QMessageBox, QGridLayout, QLabel, QVBoxLayout, QHBoxLayout
)
from PyQt5.QtGui import QFont
import pyqtgraph as pg
import socketserver
import threading
import queue


class Application(QMainWindow):
    def __init__(self):
        super().__init__()
        self.initUI()
        self.timer = QTimer(self)
    def initUI(self):
        self.resize(1000, 400)
        self.center()
        self.setWindowTitle('ECG SmallNet Classifier')
        # plot widget
        self.plotWidget = pg.PlotWidget(self)
        self.plot = self.plotWidget.plot(pen=pg.mkPen('r', width=2))
        self.plotWidget.setLabel('left', '', 'mV')
        self.setCentralWidget(self.plotWidget)
        # dock widget
        self.HRLcd = QLCDNumber(self)
        self.NormalLcd = QLCDNumber(self)
        self.AbNormalLcd = QLCDNumber(self)
        self.UnknowLcd = QLCDNumber(self)

        dock = QDockWidget("HR", self)
        dock.setWidget(self.HRLcd)
        self.addDockWidget(Qt.LeftDockWidgetArea, dock)

        dock = QDockWidget('Index', self)
        w1 = QWidget(self)
        vbox = QVBoxLayout()
        vbox.addWidget(self.createLabel('Normal'), 0, Qt.AlignTop)
        vbox.addWidget(self.NormalLcd, 5)
        w1.setLayout(vbox)

        w2 = QWidget(self)
        vbox = QVBoxLayout()
        vbox.addWidget(self.createLabel('Abnormal'),0, Qt.AlignTop)
        vbox.addWidget(self.AbNormalLcd, 4)
        w2.setLayout(vbox)

        w3 = QWidget(self)
        vbox = QVBoxLayout()
        vbox.addWidget(self.createLabel('Unknown'), 0, Qt.AlignTop)
        vbox.addWidget(self.UnknowLcd, 3)
        w3.setLayout(vbox)
        
        hbox = QHBoxLayout()
        hbox.addWidget(w1)
        hbox.addWidget(w2)
        hbox.addWidget(w3)
        w = QWidget(self)
        w.setLayout(hbox)
        
        dock.setWidget(w)
        self.addDockWidget(Qt.BottomDockWidgetArea, dock)

        self.statusBar()

        
        self.show()

    def center(self):
        qr = self.frameGeometry()
        cp = QDesktopWidget().availableGeometry().center()
        qr.moveCenter(cp)
        self.move(qr.topLeft())
    
    def createLabel(self, label):
        label = QLabel(label)
        label.setFont(QFont("Arial", 24, QFont.Bold))
        label.setAlignment(Qt.AlignHCenter | Qt.AlignBottom)
        return label

if __name__ == '__main__':
    app = QApplication(sys.argv)
    e = Application()
    sys.exit(app.exec_())