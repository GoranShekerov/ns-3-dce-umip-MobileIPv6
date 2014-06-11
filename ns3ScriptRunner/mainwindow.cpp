#include <mainwindow.h>
#include <ui_mainwindow.h>
#include <QProcess>
#include <QLineEdit>
#include <QString>
#include <iostream>
#include <QtCore>
#include <QtGui>
#include "ui_dialog.h"
#include <QMessageBox>
#include <QFile>
#include <fstream>
#include <iomanip>     //for setprecision
#include <QDateTime>

/*
 * The ns-3 (tpa module) outputs simulation results to tempresults.txt,
 * the application parse them and outputs:
 * - till_last_run_results.txt
 * - results_mean.txt
 */

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_run_pb_clicked()
{
    if (!ui->dependency_run_check->isChecked()) {run_cycle();}
    else
    {
        dep_first = (ui->first_value->text()).toInt();
        dep_last = (ui->last_value->text()).toInt();
        dep_increment = (ui->increment_value->text()).toInt();

        int dep = dep_first;
        while ( dep <= dep_last)
        {
            if ( ui->dependency_combo->currentIndex() == 0) {ui->V_value->setText(QString::number(dep));}
            if ( ui->dependency_combo->currentIndex() == 1) {ui->bN_value->setText(QString::number(dep));}
            if ( ui->dependency_combo->currentIndex() == 2) {ui->ra_interval_value->setText(QString::number(dep));}

            run_cycle();
            dep = dep + dep_increment;
        }
    }
}


void MainWindow::run_cycle()
{
    int rng;
    temp_results_array_next_index = 0;
    elapsed_time.start();

    int max_runs;
    max_runs = ui->max_run_te->text().toInt();
    if (max_runs ==1) {run (1);}

    else{
            // 5 initial runs
            for (rng = 1; rng < 6; rng++) { run (rng);}

            if ( max_runs < 6  )(max_runs = 6);
            if ( max_runs > 100)(max_runs = 100);
            while ((!check_confidence_level()) and (rng < max_runs))
            {
                run (rng);
                rng++;
            }
        }
    output_array();
    output_means();

    //  how to clear the array of structs in one line???
    for(int i=0; i < 99; i++)
    {
        temp_results_array[i].m_rngRun = 0;
        temp_results_array[i].m_dep = 0;
        temp_results_array[i].m_Th = 0;
        temp_results_array[i].m_Pl = 0;
        temp_results_array[i].m_D = 0;
        temp_results_array[i].m_J = 0;
        temp_results_array[i].m_H = 0;
        temp_results_array[i].m_Ns = 0;
        temp_results_array[i].m_Nr = 0;
        temp_results_array[i].m_Nd = 0;
        temp_results_array[i].m_T = 0;
    }
}


void MainWindow::run(int rn)
{

    run_time.start();

    q_proc.setWorkingDirectory("//root//workspace//bake//source//ns-3-dce");

    tp.m_rngRun = rn;
    ui->rngruntexted->setText(QString::number(rn));

    setArgsValues(rn); //specified in the gui
    composeCommandLine();
    q_proc.start(command, argument);
    q_proc.waitForFinished(1200000); //20min max = 1200000msec

    //Parse results
    QFile temp_results_file("//root//workspace//bake//source//ns-3-dce//tempresults.txt");
    if(temp_results_file.open( QIODevice::ReadOnly))
    {
        temp_results_str = temp_results_file.readAll();
        temp_results_file.close();
        if (parse_temp_results(temp_results_str))
        {
            //import results into array
            temp_results_array[temp_results_array_next_index] = tp;
            display_results();
            temp_results_array_next_index ++;
        }
    }

    // clear tp?

    // pause for a seconds
    QEventLoop loop;
    QTimer::singleShot(1000, &loop, SLOT(quit()));
    loop.exec();

    ui->elapsed_min_le-> setText(QString::number(elapsed_time.elapsed() / 1000 / 60));
    ui->last_run_min_le->setText(QString::number(run_time.elapsed() / 1000));
    ui->max_left_min_le->setText(QString::number(((run_time.elapsed() / 1000) * (100 - rn)) / 60));
    run_time.restart();
}


bool MainWindow::check_confidence_level()
{
    // for confidence interval of 95% and max error of 5% from from the last temp mean

// Throughput
    // calculate mean Tp
    double tp_mean; double tp_sum=0;
    for (int i = 0; i < temp_results_array_next_index; i++ )
      {tp_sum = tp_sum + temp_results_array[i].m_Th;}
    tp_mean = tp_sum / (temp_results_array_next_index);
    std::cout << "tp_mean=" << (QString::number(tp_mean)).toStdString() << std::endl;

    //calculate standard deviation
    double tp_std_dev; double tp_sumvariance = 0;
    for (int i = 0; i < temp_results_array_next_index; i++ )
      {tp_sumvariance = tp_sumvariance + (pow(temp_results_array[i].m_Th - tp_mean, 2));}
    //std::cout << "tp_sumvariance=" << (QString::number(tp_sumvariance)).toStdString() << std::endl;
    tp_std_dev = sqrt((tp_sumvariance)/(temp_results_array_next_index - 1));
    std::cout << "tp_std_dev=" << (QString::number(tp_std_dev)).toStdString() << std::endl;

    // calculate confidence interval  (5% from the mean value)
    double tp_confidence_interval;
    tp_confidence_interval= tp_mean * 0.05;
    std::cout << "tp_confidence_interval=" << (QString::number(tp_confidence_interval)).toStdString() << std::endl;

    // calculate current run error
    double tp_error;
    tp_error= 1.96 * tp_std_dev / (sqrt (temp_results_array_next_index));
    std::cout << "tp_error=" << (QString::number(tp_error)).toStdString() << std::endl;

    //set conf_table tp
    ui->confidence_table->setItem(0,0,new QTableWidgetItem(QString::number(tp_error, 'f', 2 )));
    ui->confidence_table->setItem(0,1,new QTableWidgetItem(QString::number(tp_confidence_interval, 'f', 2)));

    // is confidence level Ok
    bool tp_95 = (tp_error <= tp_confidence_interval) ? true : false;
    if (tp_95)
    {ui->confidence_table->item(0,0)->setBackgroundColor( Qt::green );
     ui->confidence_table->item(0,1)->setBackgroundColor( Qt::green );}

    temp_results_array_mean[0].m_Th = tp_mean;

// Packet loss [%]

    // calculate mean
    double pl_mean; double pl_sum=0;
    for (int i = 0; i < temp_results_array_next_index; i++ )
        {pl_sum = pl_sum + temp_results_array[i].m_Pl;}
    pl_mean = pl_sum / (temp_results_array_next_index);

    //calculate standard deviation
    double pl_std_dev; double pl_sumvariance = 0;
    for (int i = 0; i < temp_results_array_next_index; i++ )
        {pl_sumvariance = pl_sumvariance + (pow(temp_results_array[i].m_Pl - pl_mean, 2));}
    pl_std_dev = sqrt((pl_sumvariance)/((temp_results_array_next_index) - 1));

    // calculate confidence interval  (5% from the mean value)
    double pl_confidence_interval = pl_mean * 0.05;

    // calculate current run error
    double pl_error = 1.96 * pl_std_dev / (sqrt (temp_results_array_next_index));

    //set conf_table tp
    ui->confidence_table->setItem(1,0,new QTableWidgetItem(QString::number(pl_error, 'f', 2 )));
    ui->confidence_table->setItem(1,1,new QTableWidgetItem(QString::number(pl_confidence_interval, 'f', 2)));

    // is confidence level Ok
    bool pl_95 = (pl_error - 0.05 <= pl_confidence_interval) ? true : false;
    if (pl_95)
    {
        ui->confidence_table->item(1,0)->setBackgroundColor( Qt::green );
        ui->confidence_table->item(1,1)->setBackgroundColor( Qt::green );
    }
    temp_results_array_mean[0].m_Pl = pl_mean;

// Delay [ms]

    // calculate mean
    double d_mean; double d_sum=0;
    for (int i = 0; i < temp_results_array_next_index; i++ )
        {d_sum = d_sum + temp_results_array[i].m_D;}
    d_mean = d_sum / (temp_results_array_next_index);

    //calculate standard deviation
    double d_std_dev; double d_sumvariance = 0;
    for (int i = 0; i < temp_results_array_next_index; i++ )
        {d_sumvariance = d_sumvariance + (pow(temp_results_array[i].m_D - d_mean, 2));}
    d_std_dev = sqrt((d_sumvariance)/((temp_results_array_next_index) - 1));

    // calculate confidence interval  (5% from the mean value)
    double d_confidence_interval = d_mean * 0.05;

    // calculate current run error
    double d_error = 1.96 * d_std_dev / (sqrt (temp_results_array_next_index));

    //set conf_table tp
    ui->confidence_table->setItem(2,0,new QTableWidgetItem(QString::number(d_error, 'f', 2 )));
    ui->confidence_table->setItem(2,1,new QTableWidgetItem(QString::number(d_confidence_interval, 'f', 2)));

    // is confidence level Ok
    bool d_95 = (d_error <= d_confidence_interval) ? true : false;
    if (d_95)
    {
        ui->confidence_table->item(2,0)->setBackgroundColor( Qt::green );
        ui->confidence_table->item(2,1)->setBackgroundColor( Qt::green );
    }
    temp_results_array_mean[0].m_D = d_mean;

// Jitter [ms]

    // calculate mean
    double j_mean; double j_sum=0;
    for (int i = 0; i < temp_results_array_next_index; i++ )
        {j_sum = j_sum + temp_results_array[i].m_J;}
    j_mean = j_sum / (temp_results_array_next_index);

    //calculate standard deviation
    double j_stj_dev; double j_sumvariance = 0;
    for (int i = 0; i < temp_results_array_next_index; i++ )
        {j_sumvariance = j_sumvariance + (pow(temp_results_array[i].m_J - j_mean, 2));}
    j_stj_dev = sqrt((j_sumvariance)/((temp_results_array_next_index) - 1));

    // calculate confidence interval  (5% from the mean value)
    double j_confidence_interval = j_mean * 0.05;

    // calculate current run error
    double j_error = 1.96 * j_stj_dev / (sqrt (temp_results_array_next_index));

    //set conf_table tp
    ui->confidence_table->setItem(3,0,new QTableWidgetItem(QString::number(j_error, 'f', 2 )));
    ui->confidence_table->setItem(3,1,new QTableWidgetItem(QString::number(j_confidence_interval, 'f', 2)));

    // is confidence level Ok
    // ** allowed - 0.05 additional error in order not to run 70 laps for "nothing" :)
    bool j_95 = (j_error - 0.05 <= j_confidence_interval) ? true : false;
    if (j_95)
    {
        ui->confidence_table->item(3,0)->setBackgroundColor( Qt::green );
        ui->confidence_table->item(3,1)->setBackgroundColor( Qt::green );
    }
    temp_results_array_mean[0].m_J = j_mean;


// Handover delay [s]

    // calculate mean
    double h_mean; double h_sum=0;
    for (int i = 0; i < temp_results_array_next_index; i++ )
        {h_sum = h_sum + temp_results_array[i].m_H;}
    h_mean = h_sum / (temp_results_array_next_index);

    //calculate standard deviation
    double h_sth_dev; double h_sumvariance = 0;
    for (int i = 0; i < temp_results_array_next_index; i++ )
        {h_sumvariance = h_sumvariance + (pow(temp_results_array[i].m_H - h_mean, 2));}
    h_sth_dev = sqrt((h_sumvariance)/((temp_results_array_next_index) - 1));

    // calculate confidence interval  (5% from the mean value)
    double h_confidence_interval = h_mean * 0.05;

    // calculate current run error
    double h_error = 1.96 * h_sth_dev / (sqrt (temp_results_array_next_index));

    //set conf_table tp
    ui->confidence_table->setItem(4,0,new QTableWidgetItem(QString::number(h_error, 'f', 2 )));
    ui->confidence_table->setItem(4,1,new QTableWidgetItem(QString::number(h_confidence_interval, 'f', 2)));

    // is confidence level Ok
    bool h_95 = (h_error <= h_confidence_interval) ? true : false;
    if (h_95)
    {
        ui->confidence_table->item(4,0)->setBackgroundColor( Qt::green );
        ui->confidence_table->item(4,1)->setBackgroundColor( Qt::green );
    }
    temp_results_array_mean[0].m_H = h_mean;


// R value

    // calculate mean
    double r_mean; double r_sum=0;
    for (int i = 0; i < temp_results_array_next_index; i++ )
        {r_sum = r_sum + temp_results_array[i].m_R;}
    r_mean = r_sum / (temp_results_array_next_index);

    //calculate standard deviation
    double r_str_dev; double r_sumvariance = 0;
    for (int i = 0; i < temp_results_array_next_index; i++ )
        {r_sumvariance = r_sumvariance + (pow(temp_results_array[i].m_R - r_mean, 2));}
    r_str_dev = sqrt((r_sumvariance)/((temp_results_array_next_index) - 1));

    // calculate confidence interval  (5% from the mean value)
    double r_confidence_interval = r_mean * 0.05;

    // calculate current run error
    double r_error = 1.96 * r_str_dev / (sqrt (temp_results_array_next_index));

    //set conf_table tp
    ui->confidence_table->setItem(5,0,new QTableWidgetItem(QString::number(r_error, 'f', 2 )));
    ui->confidence_table->setItem(5,1,new QTableWidgetItem(QString::number(r_confidence_interval, 'f', 2)));

    // is confidence level Ok
    bool r_95 = (r_error <= r_confidence_interval) ? true : false;
    if (r_95)
    {
        ui->confidence_table->item(5,0)->setBackgroundColor( Qt::green );
        ui->confidence_table->item(5,1)->setBackgroundColor( Qt::green );
    }
    temp_results_array_mean[0].m_R = r_mean;


//-------
    for (int i = 0; i <= 5; i++){
        for (int j = 0; j<=1; j++){
            ui->confidence_table->item(i,j)->setTextAlignment(Qt::AlignCenter);}
    }


    return (tp_95 and pl_95 and d_95 and j_95 and h_95 and r_95) ? true : false;
}


void MainWindow::output_array()
{
    //Output result to file (for analising)
    std::ofstream r_temp_out("/root/workspace/bake/source/ns-3-dce/till_last_run_results.txt");

    r_temp_out << "#" << "RngRun\t" << dependency.toStdString() << "\t" << "T[Kbps]\t" << "Pl[%]\t" << "D[ms]\t" << "J[ms]\t" << "H[s]\t" << "R\t"<< "Ns\t" << "Nr\t" << "Nd\t" << "T[s]" << "\n";

    for (int i = 0; i < temp_results_array_next_index; i++)
    {
        r_temp_out << temp_results_array[i].m_rngRun    << "\t";
        r_temp_out << temp_results_array[i].m_dep       << "\t";
        r_temp_out << temp_results_array[i].m_Th        << "\t";
        r_temp_out << temp_results_array[i].m_Pl        << "\t";
        r_temp_out << temp_results_array[i].m_D         << "\t";
        r_temp_out << temp_results_array[i].m_J         << "\t";
        r_temp_out << temp_results_array[i].m_H         << "\t";
        r_temp_out << temp_results_array[i].m_R         << "\t";
        r_temp_out << temp_results_array[i].m_Ns        << "\t";
        r_temp_out << temp_results_array[i].m_Nr        << "\t";
        r_temp_out << temp_results_array[i].m_Nd        << "\t";
        r_temp_out << temp_results_array[i].m_T         << std::endl;
    }

}


void MainWindow::output_means()
{
    //Output result to file (for analising)
    std::ofstream r_temp_out("/root/workspace/bake/source/ns-3-dce/results_mean.txt", std::ios_base::app);
    if (output_label_enable == "true"){
        r_temp_out << "#" << dependency.toStdString() << "\t" << "T[Kbps]\t" << "Pl[%]\t" << "D[ms]\t" << "J[ms]\t" << "H[s]\t" << "R\t" << "\n";
        ui->output_label_check->setChecked(false);}

    r_temp_out << std::fixed << std::setprecision(2) << temp_results_array[0].m_dep     << "\t";
    r_temp_out << std::fixed << std::setprecision(2) << temp_results_array_mean[0].m_Th << "\t";
    r_temp_out << std::fixed << std::setprecision(2) << temp_results_array_mean[0].m_Pl << "\t";
    r_temp_out << std::fixed << std::setprecision(2) << temp_results_array_mean[0].m_D  << "\t";
    r_temp_out << std::fixed << std::setprecision(2) << temp_results_array_mean[0].m_J  << "\t";
    r_temp_out << std::fixed << std::setprecision(2) << temp_results_array_mean[0].m_H  << "\t";
    r_temp_out << std::fixed << std::setprecision(0) << temp_results_array_mean[0].m_R    << std::endl;
}


void MainWindow::setArgsValues(int rng)
{
    // rngRun
    rngRun = QString::number(rng);
    // mn_movement
    if (ui->movement_value->currentIndex() == 0) { mn_movement = "2";} //mobile
    if (ui->movement_value->currentIndex() == 1) { mn_movement = "1";} //static
    // mn_path
    if (ui->path_value->currentIndex() == 0) {mn_path = "1";} //HA->FN2    
    if (ui->path_value->currentIndex() == 1) {mn_path = "2";} //FN2->FN3
    if (ui->path_value->currentIndex() == 2) {mn_path = "3";} //FN2->HA
    // mn_location
    if (ui->mn_location_value->currentIndex() == 0) {mn_location = "1";} //HA
    if (ui->mn_location_value->currentIndex() == 1) {mn_location = "2";} //FN2
    if (ui->mn_location_value->currentIndex() == 2) {mn_location = "3";} //FN3
    // V
    V = ui->V_value->text();
    // traffic_type
    if (ui->pingRadio->     isChecked()) {traffic_type = "1";}
    if (ui->udpcbrRadio->   isChecked()) {traffic_type = "2";}
    if (ui->tcpcbrRadio->   isChecked()) {traffic_type = "3";}
    if (ui->voipRadio->     isChecked()) {traffic_type = "4";}
    if (ui->livevideoRadio->isChecked()) {traffic_type = "5";}
    if (ui->videoRadio->    isChecked()) {traffic_type = "6";}
    // ra_interval
    ra_interval = ui->ra_interval_value->text();
    // ro_enable
    if (ui->ro_enable_check->isChecked())  {ro_enable = "true";}
    if (!ui->ro_enable_check->isChecked()) {ro_enable = "false";}
    // background_nodes
    if (ui->background_check->isChecked())  {background_nodes = "true";}
    if (!ui->background_check->isChecked()) {background_nodes = "false";}
    // bN
    bN = ui->bN_value->text();
    // callbacks_enable
    if (ui->callbacks_check->isChecked())  {callbacks_enable = "true";}
    if (!ui->callbacks_check->isChecked()) {callbacks_enable = "false";}
    // output_label_enable
    if (ui->output_label_check->isChecked())  {output_label_enable = "true";}
    if (!ui->output_label_check->isChecked()) {output_label_enable = "false";}
    // print_throughput
    if (ui->print_throughput_check->isChecked())  {print_throughput = "true";}
    if (!ui->print_throughput_check->isChecked()) {print_throughput = "false";}
    // pcap_enable
    if (ui->pcap_check->isChecked())  {pcap_enable = "true";}
    if (!ui->pcap_check->isChecked()) {pcap_enable = "false";}
    // anim_enable
    if (ui->anim_enable_check->isChecked())  {anim_enable = "true";}
    if (!ui->anim_enable_check->isChecked()) {anim_enable = "false";}
    // dependency_enable
    if (ui->dependency_run_check->isChecked())  {dependency_enable = "true";}
    if (!ui->dependency_run_check->isChecked()) {dependency_enable = "false";}
    // dependency
    if (ui->dependency_combo->currentIndex() == 0) {dependency = "V[k/h]"; dependency_value = ui->V_value->text();}
    if (ui->dependency_combo->currentIndex() == 1) {dependency = "bN"; dependency_value = ui->bN_value->text();}
    if (ui->dependency_combo->currentIndex() == 2) {dependency = "Ra [s]";dependency_value = ui->ra_interval_value->text();}
}


void MainWindow::composeCommandLine()
{
    command = "./waf";

    QString arg_com;
    QString temp_arg_com;

    temp_arg_com = "mipv6test";           arg_com.append(temp_arg_com); temp_arg_com.clear();

    temp_arg_com = " --RngRun=";          arg_com.append(temp_arg_com); temp_arg_com.clear();
    temp_arg_com = rngRun;                arg_com.append(temp_arg_com); temp_arg_com.clear();

    temp_arg_com = " --mn_movement=";     arg_com.append(temp_arg_com); temp_arg_com.clear();
    temp_arg_com = mn_movement;           arg_com.append(temp_arg_com); temp_arg_com.clear();

    temp_arg_com = " --mn_path=";         arg_com.append(temp_arg_com); temp_arg_com.clear();
    temp_arg_com = mn_path;               arg_com.append(temp_arg_com); temp_arg_com.clear();

    temp_arg_com = " --mn_location=";     arg_com.append(temp_arg_com); temp_arg_com.clear();
    temp_arg_com = mn_location;           arg_com.append(temp_arg_com); temp_arg_com.clear();

    temp_arg_com = " --V=";               arg_com.append(temp_arg_com); temp_arg_com.clear();
    temp_arg_com = V;                     arg_com.append(temp_arg_com); temp_arg_com.clear();

    temp_arg_com = " --traffic_type=";    arg_com.append(temp_arg_com); temp_arg_com.clear();
    temp_arg_com = traffic_type;          arg_com.append(temp_arg_com); temp_arg_com.clear();

    temp_arg_com = " --ra_interval=";     arg_com.append(temp_arg_com); temp_arg_com.clear();
    temp_arg_com = ra_interval;           arg_com.append(temp_arg_com); temp_arg_com.clear();

    temp_arg_com = " --ro_enable=";       arg_com.append(temp_arg_com); temp_arg_com.clear();
    temp_arg_com = ro_enable;             arg_com.append(temp_arg_com); temp_arg_com.clear();

    temp_arg_com = " --background_nodes=";arg_com.append(temp_arg_com); temp_arg_com.clear();
    temp_arg_com = background_nodes;      arg_com.append(temp_arg_com); temp_arg_com.clear();

    temp_arg_com = " --bN=";              arg_com.append(temp_arg_com); temp_arg_com.clear();
    temp_arg_com = bN;                    arg_com.append(temp_arg_com); temp_arg_com.clear();

    temp_arg_com = " --callbacks_enable=";arg_com.append(temp_arg_com); temp_arg_com.clear();
    temp_arg_com = callbacks_enable;      arg_com.append(temp_arg_com); temp_arg_com.clear();

    temp_arg_com = " --output_label_enable=";arg_com.append(temp_arg_com); temp_arg_com.clear();
    temp_arg_com = output_label_enable;      arg_com.append(temp_arg_com); temp_arg_com.clear();

    temp_arg_com = " --print_throughput=";arg_com.append(temp_arg_com); temp_arg_com.clear();
    temp_arg_com = print_throughput;      arg_com.append(temp_arg_com); temp_arg_com.clear();

    temp_arg_com = " --pcap_enable=";     arg_com.append(temp_arg_com); temp_arg_com.clear();
    temp_arg_com = pcap_enable;           arg_com.append(temp_arg_com); temp_arg_com.clear();

    temp_arg_com = " --anim_enable=";     arg_com.append(temp_arg_com); temp_arg_com.clear();
    temp_arg_com = anim_enable;           arg_com.append(temp_arg_com); temp_arg_com.clear();


    argument << "--run" << arg_com;

}


bool MainWindow::parse_temp_results(QString str)
{
    //m_rngRun - set in ::run(int rn)
    tp.m_dep = dependency_value.toInt();
    tp.m_Th = (str.section("*",0,0)).toDouble();
    tp.m_Pl = (str.section("*",1,1)).toDouble();
    tp.m_D  = (str.section("*",2,2)).toDouble();
    tp.m_J  = (str.section("*",3,3)).toDouble();
    tp.m_H  = (str.section("*",4,4)).toDouble();
    tp.m_R  = (str.section("*",5,5)).toInt();
    tp.m_Ns = (str.section("*",6,6)).toInt();
    tp.m_Nr = (str.section("*",7,7)).toInt();
    tp.m_Nd = (str.section("*",8,8)).toInt();
    tp.m_T  = (str.section("*",9,9)).toInt();

    //return ((tp.m_Pl > 0) and (tp.m_D > 0) and (tp.m_J > 0) and (tp.m_H >= 0) and (tp.m_R > 0)) ? true : false;
    return ((tp.m_Pl >= 0) and (tp.m_D > 0) and (tp.m_J > 0) and (tp.m_H >= 0) and (tp.m_R > 0)) ? true : false;
}


void MainWindow::display_results()
{
    QString temp;
    temp = QString::number(tp.m_rngRun);  ui->display_param_table->setItem(0,0,new QTableWidgetItem(temp));
    temp = dependency;                    ui->display_param_table->setHorizontalHeaderItem(1, new QTableWidgetItem(temp));
    temp = QString::number (tp.m_dep);    ui->display_param_table->setItem(0,1,new QTableWidgetItem(temp));
    temp = QString::number (tp.m_Th);     ui->display_param_table->setItem(0,2,new QTableWidgetItem(temp));
    temp = QString::number (tp.m_Pl);     ui->display_param_table->setItem(0,3,new QTableWidgetItem(temp));
    temp = QString::number (tp.m_D);      ui->display_param_table->setItem(0,4,new QTableWidgetItem(temp));
    temp = QString::number (tp.m_J);      ui->display_param_table->setItem(0,5,new QTableWidgetItem(temp));
    temp = QString::number (tp.m_H);      ui->display_param_table->setItem(0,6,new QTableWidgetItem(temp));
    temp = QString::number (tp.m_R);      ui->display_param_table->setItem(0,7,new QTableWidgetItem(temp));
    temp = QString::number (tp.m_Ns);     ui->display_param_table->setItem(0,8,new QTableWidgetItem(temp));
    temp = QString::number (tp.m_Nr);     ui->display_param_table->setItem(0,9,new QTableWidgetItem(temp));
    temp = QString::number (tp.m_Nd);     ui->display_param_table->setItem(0,10,new QTableWidgetItem(temp));
    temp = QString::number (tp.m_T);      ui->display_param_table->setItem(0,11,new QTableWidgetItem(temp));

    for (int i = 0; i < 12; i++){ui->display_param_table->item(0,i)->setTextAlignment(Qt::AlignCenter);}
}


void MainWindow::on_movement_value_currentIndexChanged(int index)
{
    if (index == 0)
      {
        ui->path_value->setDisabled(false);
        ui->label_4->setDisabled(false);
        ui->V_value->setDisabled(false);
        //ui->V_value->setText("15");
        ui->label->setDisabled(false);
        ui->mn_location_value->setDisabled(true);
        ui->label_5->setDisabled(true);
      }
    if (index == 1)
      {
        ui->path_value->setDisabled(true);
        ui->label_4->setDisabled(true);        
        // if mn_movement = static => V=0
        if (ui->movement_value->currentIndex() == 1) {ui->V_value->setText("0");}
        ui->V_value->setDisabled(true);
        //ui->V_value->setText("0");
        ui->label->setDisabled(true);
        ui->mn_location_value->setDisabled(false);
        ui->label_5->setDisabled(false);
      }
}


void MainWindow::on_dependency_run_check_clicked(bool checked)
{
    if (checked)
      {
        //ui->label_6->setDisabled(false);
        ui->label_7->setDisabled(false);
        ui->label_8->setDisabled(false);
        ui->label_9->setDisabled(false);
        //ui->dependency_combo->setDisabled(false);
        ui->first_value->setDisabled(false);
        ui->last_value->setDisabled(false);
        ui->increment_value->setDisabled(false);
      } else
          {
            //ui->label_6->setDisabled(true);
            ui->label_7->setDisabled(true);
            ui->label_8->setDisabled(true);
            ui->label_9->setDisabled(true);
            //ui->dependency_combo->setDisabled(true);
            ui->first_value->setDisabled(true);
            ui->last_value->setDisabled(true);
            ui->increment_value->setDisabled(true);
          }
}


void MainWindow::on_background_check_clicked(bool checked)
{
    if (checked)
      {
        ui->label_12->setDisabled(false);
        ui->bN_value->setDisabled(false);
      } else
          {
            ui->label_12->setDisabled(true);
            ui->bN_value->setDisabled(true);
          }
}


void MainWindow::on_clear_results_pb_clicked()
{
    QDateTime datetime = QDateTime::currentDateTime();
    QString datetime_s = datetime.toString();
    datetime_s.replace(QString(" "), QString("_"));
    datetime_s.prepend("results_mean_"); datetime_s.append(".txt");
    //std::cout << datetime_s.toStdString() << std::endl;

    q_proc.setWorkingDirectory("//root//workspace//bake//source//ns-3-dce");
    QStringList arg;
    arg << "results_mean.txt" << datetime_s;
    q_proc.start("cp", arg);
    q_proc.waitForFinished();
    q_proc.start("rm", QStringList("results_mean.txt"));
    q_proc.waitForFinished();
    q_proc.start("rm", QStringList("till_last_run_results.txt"));
    q_proc.waitForFinished();

    system("cd //root//workspace//bake//source//ns-3-dce; rm *.pcap");
}


void MainWindow::on_open_graphs_b_clicked()
{
    q_proc.start("nautilus", QStringList("//root//workspace//bake//source//ns-3-dce//plot"));
}


void MainWindow::on_plot_graphs_b_clicked()
{
    system("cp //root//workspace//bake//source//ns-3-dce//results_mean.txt //root//workspace//bake//source//ns-3-dce//plot//results_mean.txt");
    system("cp //root//workspace//bake//source//ns-3-dce//Throughput.txt   //root//workspace//bake//source//ns-3-dce//plot//Throughput.txt");

    system("cd //root//workspace//bake//source//ns-3-dce//plot; ./run_mipv6gnupl.sh");

    // pause for a seconds
    QEventLoop loop;
    QTimer::singleShot(5000, &loop, SLOT(quit()));
    loop.exec();
}


void MainWindow::on_clear_graphs_pb_clicked()
{
    system("cd //root//workspace//bake//source//ns-3-dce//plot; rm *.gif");
}


// trash:)
// ui->textEdit->setPlainText(temp_results_str);
// print last result in window
// QString StdOut = q_proc.readAllStandardOutput();
// ui->textEdit->setPlainText(display_results(temp_results_str));
// std::cout << StdOut.toStdString() << std::endl;
