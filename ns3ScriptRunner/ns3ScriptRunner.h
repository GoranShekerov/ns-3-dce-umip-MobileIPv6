#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QProcess>
#include <QVector>
#include <QTime>


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_run_pb_clicked();

    void on_movement_value_currentIndexChanged(int index);

    void on_dependency_run_check_clicked(bool checked);

    void on_background_check_clicked(bool checked);



    void on_clear_results_pb_clicked();

    void on_open_graphs_b_clicked();

    void on_plot_graphs_b_clicked();

    void on_clear_graphs_pb_clicked();

private:
    void run_cycle();
    void setArgsValues(int rng);
    void composeCommandLine();
    bool parse_temp_results(QString str);
    void display_results();
    void run(int rn);
    bool check_confidence_level();
    void output_array();
    void output_means();


    QProcess q_proc;
    QString temp_results_str;


    Ui::MainWindow *ui;

    QString command;
    QStringList argument;


    //args
    QString rngRun;
    QString mn_movement;
    QString mn_path;
    QString mn_location;
    QString V;
    QString traffic_type;
    QString ra_interval;
    QString ro_enable;
    QString background_nodes;
    QString bN;
    QString callbacks_enable;
    QString output_label_enable;
    QString print_throughput;
    QString pcap_enable;
    QString anim_enable;
    QString dependency_enable;
    QString dependency;
    QString dependency_value;
    int dep_first;
    int dep_last;
    int dep_increment;

    // Traffic parameters
    // Th[Kbps]  Pl[%]   D[ms]   J[ms]   H[s]    R       Ns      Nr      Nd      T[s]
    struct trafficParam
    {
        int    m_rngRun;
        int    m_dep;
        double m_Th;
        double m_Pl;
        double m_D;
        double m_J;
        double m_H;
        int    m_R;
        int    m_Ns;
        int    m_Nr;
        int    m_Nd;
        int    m_T;
    };

    trafficParam tp;
    trafficParam temp_results_array[100];
    int temp_results_array_next_index;
    trafficParam temp_results_array_mean[1];

    QTime elapsed_time;
    QTime run_time;
};

#endif // MAINWINDOW_H
