#include "mainwindow.h"
#include <DTitlebar>
#include <QLabel>
#include <QDebug>
#include <QTimer>
#include <QLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QStatusBar>
#include <QMenu>
#include <QAction>
#include <QMenuBar>
#include <QToolBar>
#include <QIcon>
#include <QFileDialog>
#include <QSqlDatabase>
#include <QMessageBox>
#include <QModelIndex>
#include <QDesktopWidget>
#include <QDesktopServices>
#include <QMimeData>
#include <QLocalSocket>
#include <QCoreApplication>
#include <QProcess>
#include <QClipboard>
#include <QThread>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "sqlitefunt.h"
#include "interfaceadaptor.h"

/** */
#include "dthememanager.h"
#include "utils.h"

#define  SLEEPSS          2000

MainWindow::MainWindow(DMainWindow *parent) :DMainWindow( parent){

    initMainWindow();
}

//removeDownloadResult //purgeDownloadResult
MainWindow::~MainWindow(){

    qDebug() << "~MainWindow()";

    delete this->downDB;
    delete this->aria2c;
    delete this->newDownDlg;

}

void MainWindow::initMainWindow(){

    /**
     *  初始化 aria2c 接口
     */
    initAria2cWork();
    actionResult =  "http://localhost:19799/jsonrpc";
    this->aria2c = new Aria2cRPCMsg( actionResult ,this  );

    /**
     * 初始化 sqlite 数据库
     */
    qDebug() << "QSqlDatabase::drivers " <<QSqlDatabase::drivers();
    this->downDB  = new SQLiteFunt( "deepin_download.db" );

    /**
     * 初始化 剪贴板
     */
    board = QApplication::clipboard();

    /**
     * 主界面样式 & 布局
     */
    DThemeManager::instance()->setTheme("light");
    installEventFilter(this);

    layoutWidget = new QWidget();    
    QVBoxLayout *mainlayout = new QVBoxLayout( layoutWidget );
    mainlayout->setContentsMargins(0, 0, 0, 10);

    layout = new QHBoxLayout();

    slidebar = new SlideBar();        //左侧边
    centerWidget = new QWidget;       //中部容器（ 表格 ）
    LoadTableView( centerWidget );    //中部主体表格

    layout->addWidget(slidebar);
    layout->addWidget(centerWidget );

    toolbar = new ToolBar();          //工具栏
    this->titlebar()->setCustomWidget(toolbar, Qt::AlignVCenter, false);

    QHBoxLayout *bottomlayout = new QHBoxLayout;  // 底部栏
    bottomlabel = new QLabel;
    bottomlayout->addStretch();
    bottomlayout->addWidget( bottomlabel );
    bottomlayout->addStretch();

    mainlayout->addLayout(  layout );
    mainlayout->addLayout( bottomlayout );
    this->setCentralWidget(  layoutWidget );


    /** 加载托盘 */
    LoadTrayIcon();                 //系统托盘图标及菜单

    /** 下载对话框 （ uri ）*/
    newDownDlg =  new NewDown( this );

    /** 选项配置 */
    configDlg  =   new ConfigDlg;

    /** 关于 */
    aboutDlg  = new AboutDlg;

    /** 浮窗 */
    /***
    mwm  = new MWM( this );
    mwm->ShowMWM();
    ***/

    /** 启用文件拖入支持 */
    this->setAcceptDrops( true );

    /**
     * 初始化计时器
     */
    m_Active = new QTimer( this );
    m_Stop   = new QTimer( this );
    m_Wait   = new QTimer( this );
    m_All    = new QTimer( this );

    m_Active->setInterval( SLEEPSS );
    m_Stop->setInterval( SLEEPSS );
    m_Wait->setInterval( SLEEPSS );
    m_All->setInterval( SLEEPSS );

    connect( m_Active, SIGNAL(timeout()), this, SLOT( GetActiveList()) );
    connect( m_Stop, SIGNAL(timeout()), this, SLOT( GetStopList()) );
    connect( m_Wait, SIGNAL(timeout()), this, SLOT( GetWaitList()) );
    connect( m_All, SIGNAL(timeout()), this, SLOT( UpdateDownStatus()) );

    /**
     * 左侧边栏监听
     * 顶工具栏监听
     **/
    connect( slidebar, SIGNAL(SelSlideItem( int)), this, SLOT( SelSlideItem( int) ) );
    connect( toolbar, SIGNAL(SelToolItem( int)), this, SLOT( SelToolItem( int) ) );

    connect( toolbar, SIGNAL(SearchChang( QString )), this, SLOT( SearchChang( QString ) ) );


    //////////////////////////////////////////////////////////////////////////////////////////

    /**
     * 开启 状态刷新
     */
    //m_All->start();


    /**
     * 系统关于菜单
     */
    setupMenu();

    /**
     * 主窗口界面相关
     */
    setWindowTitle(tr("深度下载"));
    setWindowIcon( QIcon(":Resources/images/logo@2x.png")  );

    /**
     * dbus 监听，接收二次发起程序时传递的下载文件名URL
     */
    new InterfaceAdaptor( this );

    /**
     * 多线程轮循
     */
    //wThread = new GCThread( this );

    /** 状态栏文本设置 */
    SetBottomStatusText("");

}

void MainWindow::setupMenu()
{
    DTitlebar *titlebar = this->titlebar();

    if ( titlebar ) {

        //titlebar->setWindowFlags(titlebar->windowFlags() & ~Qt::WindowMaximizeButtonHint);
        titlebar->setMenu( new QMenu( titlebar ) );
        titlebar->setSeparatorVisible(true);
/**
        QMenu *NM1 = titlebar->menu()->addMenu( "测试" );
        NM1->addAction( "子项1");
        NM1->addAction( "子项2");
        NM1->addAction( "子项3");
**/
        //QAction *helpItem = titlebar->menu()->addAction("帮助");
        //helpItem->setData( "help" );
        //titlebar->menu()->addAction("关于");
        //titlebar->menu()->addAction("退出");


    }
}



void MainWindow::CloseAllWindow(){


    qDebug() << "CloseAllWindow()";
    newDownDlg->close();
    aboutDlg->close();
    configDlg->close();
    //mwm->close();
}



void MainWindow::closeEvent(QCloseEvent *event){

    qDebug() << "MainWindow::close()";
    //QMainWindow::close();
    if( this->closeApp ){

        CloseAllWindow();
        event->accept();

    }else{

        /** 主窗口上关闭时 自动最小化 */
        showMinimized(); //最小化
        //隐藏窗口之后，关闭子窗口会导致程序退出？？ 原因暂不明
        //this->hide(); /** 暂时不使用关闭时自动隐藏 */
        //systemTrayIcon->ShowTrayMessage( "你去忙吧","我在这儿呢默默干活，有事我叫你..."  );
        event->ignore();
    }
}

void MainWindow::SelToolItem( int btn ){

    switch ( btn  ) {
        case 1:  // 普通URI 下载
            newDownDlg->show();
            break;
        case 2:  // BT 种子文件
            AddBtFile();
            break;
        case 3:  // Metalink 磁力链
            AddMetalinkFile();
            break;

        case 4:  // 继续
            UnPause();
            break;

        case 5:  // 暂停
            Pause();
            break;

        case 6:  // 移除
            Remove();
            break;

        default:
            break;
    }
}

void MainWindow::SelSlideItem( int row ){

    //wThread->setFunction( 0 );
    this->downListView->ClearAllItem();

    if ( m_Active->isActive() )
        m_Active->stop();

    if ( m_Stop->isActive() )
        m_Stop->stop();

    if ( m_Wait->isActive() )
        m_Wait->stop();

    if ( m_All->isActive() )
        m_All->stop();

    qDebug()<< "slideBar.currentRowChanged: " << row;

    this->SlideSelItem = row;

    toolbar->SetToolButton();

    switch ( row ) {

    case 1:   //下载中

        m_Active->start();
        //wThread->setFunction( 1 );
        break;
    case 2:   //队列中

        m_Wait->start();
        //wThread->setFunction( 3 );
        break;
    case 3:   //已完成

        m_Stop->start();
        //wThread->setFunction( 2 );
        break;

    case 4:   // 历史记录
        //wThread->setFunction( 0 );
        toolbar->SetToolSearch();
        GetDDList();
        break;

    default:  //全部任务
        //m_All->start();
        break;
    }

    //wThread->start();
}



void MainWindow::SetBottomStatusText( QString text ){


    bottomlabel->setText(  text  );
}

/**
*  每两秒向 aria2 抓取一次 状态
*  sqlite 表中记录
*/
void MainWindow::UpdateDownStatus(){

    qDebug() << "UpdateDownStatus()";

    /** 获取“未完成下载任务”的状态 */
    QList<DDRecord> t = downDB->ReadDDTask();
    for (  int i = 0; i < t.size() ;i++){

        this->aria2c->SendMsgAria2c_tellStatus( t.at(i).gid );
    }

    //SeeBoard();
}

void MainWindow::SeeBoard(){

  /**
   *  监视剪贴板 中的内容，过滤 url 下载地址
   * ============================================================== */
    QString str = board->text(); //使用text() |image() | pixmap()
    qDebug()<< "clipboard() " + str;

    QList<QString> durllist = FindDownloadUrl( str );

    for ( int i = 0 ; i < durllist.size() ; i++  ){

        if ( ! downDB->findATaskI( durllist.at(i) )  &&  ! newDownDlg->isVisible()  ){
            newDownDlg->SetDownloadEdit( durllist.at(i) );
            newDownDlg->show();
        }
    }

   /** ========================================================== */


}

/**
 *  装载托盘图标 TrayIcon
*/
void MainWindow::LoadTrayIcon(){

   systemTrayIcon = new GCSystemTrayIcon( this );

   connect( systemTrayIcon, &QSystemTrayIcon::activated, this, [=]( QSystemTrayIcon::ActivationReason reason ){

       switch (reason){
           //case QSystemTrayIcon::Context:
           //    break;
           case QSystemTrayIcon::DoubleClick:
               ShowWindow();
               break;
           //case QSystemTrayIcon::Trigger:
           //    break;
       }

   } );

   connect( systemTrayIcon->m_menu, &QMenu::triggered, this, [=]( QAction *action){

       qDebug()<< action;

       switch ( action->data().toInt() ) {
       case 1:           // 关于
           aboutDlg->show();
           break;
       case 2:           // 退出
           this->closeApp = true;
           this->close();
           break;

       case 3:           // 选项配置
           configDlg->show();
           break;
       case 4:           // 主窗口
           ShowWindow();
           break;

       case 5:
           //mwm->HideMWM();    //隐藏悬浮窗
           break;

       case 6:
           //mwm->ShowMWM();    //显示悬浮窗
           break;

       default:
           break;
       }


   });

}


/**
 * 载入表格
*/
void MainWindow::LoadTableView( QWidget *centerWidget ){

    downListView =  new DownListView( this,centerWidget );

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget( downListView );
    //layout->setContentsMargins(0,0,0,0);
    centerWidget->setLayout( layout );

    //selected = downListView->selectionModel()->selectedRows();
    connect( downListView, &QTableView::clicked,this, [=](QModelIndex modelIndex ){

          qDebug() << "downListView.click: " << modelIndex;
    });


    connect( downListView,&QTableView::doubleClicked, this,[=](QModelIndex modelIndex){

          qDebug() <<"downListView.DoubleClick: " << modelIndex;
    });


   /**
    * 右键菜单
    */
    connect( downListView->m_ContextMenu, &QMenu::triggered , this,[=]( QAction *action  ){

          qDebug() <<"downListView->m_ContextMenu.triggered: " << action;

          switch ( action->data().toInt() ) {

             case 1: // 暂停
                 Pause();
                 break;
             case 2: //继续
                 UnPause();
                 break;

             case 3: //移除
                Remove();
                break;

             case 4: //属性
                Property();
                break;

             case 5:  //重新下载
                AgainDown();
                break;

             case 6:  //打开所在文件夹
                OpenDownFile();
                break;

             case 7:  //复制下载地址
                CopyUrlToBoard();
                break;

             case 8: //删除下载记录
                DeleteDownFileDB();
                break;

             case 9: //缓存清理
                RemoveAria2Cache();
                break;

             default:
                  break;
          }

    });

}

void MainWindow::Property(){

}


/**
* 重新下载
*/
void MainWindow::AgainDown(){

    const QModelIndexList selected = downListView->selectionModel()->selectedRows();

    foreach( const QModelIndex & index, selected){

        QString gid = index.sibling( index.row() ,5 ).data().toString();

        if ( gid == ""  ) continue;

        QString classD =  downDB->GetDTaskInfo( gid ).classn;
        QString url = downDB->GetDownUrlPath( gid );
        bool  z;
        int classI = classD.toInt( &z,10 );

        switch ( classI ) {
        case  1:
            AppendDownUrl( url );
            break;
        case  2:
            AppendDownBT( url );
            break;
        case  3:
            AppendDownMetalink( url );
            break;
        default:
            break;
        }
    }
}

void MainWindow::CopyUrlToBoard(){

    const QModelIndexList selected = downListView->selectionModel()->selectedRows();
    foreach( const QModelIndex & index, selected){

        QString gid = index.sibling( index.row() ,5 ).data().toString();
        if ( gid != ""  ){

            QString URL = downDB->GetDownUrlPath( gid );
            if( URL == "" ){
              ShowMessageTip( tr("Failed to get the original address, the download record may have been deleted artificially") );
            }else{
              ShowMessageTip( URL );
              board->setText(  URL );
            }
        }
    }
}

void MainWindow::OpenDownFile(){

    const QModelIndexList selected = downListView->selectionModel()->selectedRows();
    foreach( const QModelIndex & index, selected){

        QString gid = index.sibling( index.row() ,5 ).data().toString();
        if ( gid != ""  ){

            QString SavePath = downDB->GetDownSavePath( gid );
            //ShowMessageTip( SavePath );
            QFileInfo spathInfo( SavePath );
            if( spathInfo.isFile() ){
                OpenDownFilePath( SavePath );
            }else{
                ShowMessageTip( tr("File ") + SavePath + tr(" No, it may have been deleted.") );
            }
        }
    }
}

void MainWindow::RemoveAria2Cache(){

    aria2c->SendMsgAria2c_purgeDownloadResult();
    /**
    const QModelIndexList selected = downListView->selectionModel()->selectedRows();
    foreach( const QModelIndex & index, selected){

        QString gid = index.sibling( index.row() ,5 ).data().toString();

        if ( gid != ""  ){

            aria2c->SendMsgAria2c_removeDownloadResult( gid );
            aria2c->SendMsgAria2c_purgeDownloadResult();
        }
    }
    **/
}

/**
* 删除历史记录
*/
void MainWindow::DeleteDownFileDB(){

    const QModelIndexList selected = downListView->selectionModel()->selectedRows();
    foreach( const QModelIndex & index, selected){

        QString gid = index.sibling( index.row() ,5 ).data().toString();
        if ( gid != ""  ){

            QString filePath = downDB->GetDownSavePath( gid );

            if ( ! QFile::remove( filePath ) ){

                ShowMessageTip( filePath + tr(" File deletion failure") );
            }
            downDB->DeleteDTask( gid );
        }
    }

    GetDDList();

}

void MainWindow::Remove(){

    const QModelIndexList selected = downListView->selectionModel()->selectedRows();
    foreach( const QModelIndex & index, selected){

        QString gid = index.sibling( index.row() ,5 ).data().toString();
        //QString status = index.sibling( index.row() ,4).data().toString();
        if ( gid != ""  ){
            aria2c->SendMsgAria2c_pause( gid );
            aria2c->SendMsgAria2c_remove( gid );
        }
    }
}

/**
* 暂停下载
*/
void MainWindow::Pause(){

    const QModelIndexList selected = downListView->selectionModel()->selectedRows();

    foreach( const QModelIndex & index, selected){

        QString gid = index.sibling( index.row() ,5 ).data().toString();
        QString status = index.sibling( index.row() ,4 ).data().toString();

        if ( status == "active"  ){
            aria2c->SendMsgAria2c_pause( gid );
        }
    }
}

/**
* 恢复
*/
void MainWindow::UnPause(){

    const QModelIndexList selected = downListView->selectionModel()->selectedRows();

    foreach( const QModelIndex & index, selected){

        QString gid = index.sibling( index.row() ,5 ).data().toString();
        QString status = index.sibling( index.row() ,4 ).data().toString();

        if ( status == "paused"  ){
            aria2c->SendMsgAria2c_unpause( gid );
        }
    }

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
  * 载入侧边栏
  */
void MainWindow::LoadSlideBar( QWidget *leftWidget ){
/*
    slideBar = new GCSlideBar ;

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget( slideBar );
    //layout->setContentsMargins(0,0,0,0);
    leftWidget->setLayout( layout );

    connect( slideBar, &QListWidget::currentRowChanged, this,[=]( int currentRow){

        this->downListView->ClearAllItem();

        if ( m_Active->isActive() )
            m_Active->stop();

        if ( m_Stop->isActive() )
            m_Stop->stop();

        if ( m_Wait->isActive() )
            m_Wait->stop();

        //if ( m_All->isActive() )
        //    m_All->stop();

        qDebug()<< "slideBar.currentRowChanged: " << currentRow;

        this->SlideSelItem = currentRow;

        switch ( currentRow ) {

        case 1:   //下载中

            m_Active->start();
            break;
        case 2:   //队列中

            m_Wait->start();
            break;
        case 3:   //已完成

            m_Stop->start();
            break;

        case 4:   // 历史记录

            GetDDList();
            break;

        default:  //全部任务
            //m_All->start();
            break;
        }
    });
*/
}


/**
  * 载入状态栏
  */
void MainWindow::LoadStatusbar( QWidget *bottomWidget ){

    statusBar = new GCStatusBar( bottomWidget );
    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget( statusBar );
    //layout->setContentsMargins(0,0,0,0);
    bottomWidget->setLayout( layout );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////


void MainWindow::AddBtFile(){

    QString path = QFileDialog::getOpenFileName(this,
                                                tr("Open BitTorrent File"), ".", "BTorrent Files(*.torrent)" );

    if( path.length() != 0 ) {

        qDebug() << "path " << path;        
        AppendDownBT( path );
    }
}

void MainWindow::AddMetalinkFile(){

    QString path = QFileDialog::getOpenFileName(this,
                                                tr("Open Metalink File"), ".", "Metalink File(*.metalink)");

    if( path.length() != 0 ) {

        qDebug() << "path " << path;        
        AppendDownMetalink( path );
    }

}


///////////////////////////////////////////////////////////////////////////////////////////////////////

void MainWindow::LoadDownList(){

   //

}

void MainWindow::AppendDownUrl( QString urlStr  ){

   QString ID = downDB->AppendDTask(  urlStr );
   aria2c->SendMsgAria2c_addUri( urlStr ,ID  );
   ShowMessageTip( tr("Join a new task：") + urlStr  );
   //slideBar->SetSelectRow( 1 );
}

void MainWindow::AppendDownBT( QString btfilepath   ){

    QString ID = downDB->AppendDTask(  btfilepath ,"2" );
    aria2c->SendMsgAria2c_addTorrent( btfilepath ,ID  );
    ShowMessageTip( tr("Enable：") + btfilepath  );
    //slideBar->SetSelectRow( 1 );
}

void MainWindow::AppendDownMetalink( QString Metalinkfilepath   ){

    QString ID = downDB->AppendDTask(  Metalinkfilepath ,"3" );
    aria2c->SendMsgAria2c_addMetalink( Metalinkfilepath ,ID );
    ShowMessageTip( tr("Enable： ") + Metalinkfilepath  );
    //slideBar->SetSelectRow( 1 );
}



///////////////////////////////////////////////////////////////////////////////////////////////////

/**
* 拖入文件 处理
*/
void MainWindow::dragEnterEvent( QDragEnterEvent *e ){

    qDebug() << "dragEnterEvent ";

    if( e->mimeData()->hasFormat("text/uri-list") || e->mimeData()->hasFormat("torrent/uri-list") ){
        e->acceptProposedAction();
    }

}

void MainWindow::dropEvent( QDropEvent *e ){

   qDebug() << "dropEvent ";


   QList<QUrl> urls = e->mimeData()->urls();
   if(urls.isEmpty()) return ;

   foreach ( QUrl u, urls ) {
      qDebug()<< "FileName: " <<  u.toString();
      OpenLinkFile( u.toString() );
   }

}

void MainWindow::OpenLinkFile( QString filename ){


    filename = filename.replace( "file:///" ,"/"  );

    qDebug() << "OpenLinkFile filename: " << filename;
    int first = filename.lastIndexOf (".");
    QString TypeName = filename.right( filename.length() - first - 1 );

    if (  "torrent" == TypeName   ){

        AppendDownBT( filename );

    } else if ( "metalink" == TypeName  ){

        AppendDownMetalink( filename );
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////
/** ***
 *  aria2c 获取到的状态信息刷新到 MWM
 */
void MainWindow::UpdateGUI_StatusMsg( TBItem tbitem ){

    //qDebug() << "UpdateGUI_StatusMsg " <<tbitem.savepath;

    downDB->SetDownSavePath( tbitem.gid ,tbitem.savepath );
    /** 更新进度到浮窗 */
    //UpdateMWM( tbitem.Progress );

    /** 在数据库表中记录 已完成的任务 */
    if ( tbitem.State == "complete" ){

        QString dbt_Type = downDB->GetDTaskInfo( tbitem.gid ).type;
        if ( dbt_Type != "4" ){

           DDRecord t;
           t.gid = tbitem.gid;
           t.type = "4" ;   //4 标注已完成
           downDB->SetDTaskStatus( t );
           ShowMessageTip( tr("Completed ") + tbitem.uri  );
        }
   }

}

/**
*
**/
void MainWindow::UpdateGUI_StatusMsg(  QList<TBItem>  tbList ){


    qDebug() << "UpdateGUI_StatusMsg " << "tbList.at(0)";
    /** 记录下此时 listView 中已选择的行 */
    const QModelIndexList selected = downListView->selectionModel()->selectedRows();

    downListView->ClearAllItem();


    for( int i = 0 ; i < tbList.size() ; i++  ){

        downListView->InsertItem( i,tbList.at(i) );
        //mwm->UpdateMWM(  tbList.at(i).Progress );
        downDB->SetDownSavePath( tbList.at(i).gid ,tbList.at(i).savepath );

        downDB->AppendBTask( tbList.at(i) );
    }

    /**
     * 恢复此前 listView 已选择行的高亮
     */
    QItemSelection sel;
    foreach( const QModelIndex & index, selected){
        QItemSelectionRange readSel( index );
        sel.append( readSel );
    }
    downListView->selectionModel()->select( sel ,QItemSelectionModel::Select|QItemSelectionModel::Rows );
}


void MainWindow::UpdateGUI_CommandMsg( QJsonObject nObj ){
  //
}

/**
* 从数据库中获取历史记录
**/
void MainWindow::GetDDList(){

   downListView->ClearAllItem();

   QList<DDRecord> t = this->downDB->ReadALLTask();

   for( int i = 0 ; i < t.size() ; i++ ){

       TBItem x;

       QString filename = t.at(i).url.right( t.at(i).url.length() - t.at(i).url.lastIndexOf ("/") - 1 );
       x.uri = filename;
       x.gid  = t.at(i).gid;
       x.Progress = "0";
       this->downListView->InsertItem( i,x );
   }

}

void MainWindow::GetWaitList(){

    this->aria2c->SendMsgAria2c_tellWaiting( 0 ,10 );

}

void MainWindow::GetStopList(){

    this->aria2c->SendMsgAria2c_tellStopped( 0 , 10 );

}

void MainWindow::GetActiveList(){

   this->aria2c->SendMsgAria2c_tellActive();
}


void MainWindow::ShowTrayMenu(){

    systemTrayIcon->m_menu->show();

}

void MainWindow::ShowWindow(){

    if ( ! this->isVisible()  ){

        this->show();
    }

    this->activateWindow();
}

void MainWindow::UpdateMWM( QString text ){

   /** 更新悬浮窗　*/
   //mwm->UpdateMWM( text );
}

void MainWindow::ShowMessageTip( QString text ){

   /**  右下角气泡消息窗口　暂弃用 */
   // systemTrayIcon->m_tooltip->ShowMessage( text  );
}

void MainWindow::OPenDownUrlDlg( QString DownFileUrl ){

    if ( DownFileUrl != "" ){

        newDownDlg->SetDownloadEdit( DownFileUrl );
        newDownDlg->show();
    }
}

void MainWindow::StartRun( QString DownFileUrl ){

    if ( DownFileUrl != "" ){

        newDownDlg->SetDownloadEdit( DownFileUrl );
        newDownDlg->show();
    }

    QMainWindow::show();

}

void MainWindow::OpenDownFilePath( QString filepath ){

    int first = filepath.lastIndexOf ("/");
    QString filename = filepath.right( filepath.length() - first - 1 );

    QString dir = filepath.replace( filename, "" );

    qDebug() << dir;

    QDesktopServices::openUrl(QUrl::fromLocalFile( dir ));
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

QList<QString> MainWindow::FindDownloadUrl( const QString &strText ){

    QList<QString> urlList;
    QString strTempText = strText;
    QString strUrlExp = "((http|https|ftp)://|(www)\\.)(\\w+)(\\.?[\\.a-z0-9/:?%&=\\-_+#;]*)"; //url正则

    QRegExp urlRegExp(strUrlExp,Qt::CaseInsensitive);

    while(urlRegExp.indexIn(strTempText) != -1){

        QString strWebUrl = urlRegExp.cap(0);
        qDebug() << strWebUrl;
        urlList.append( strWebUrl );
        int nIndex = strTempText.indexOf(strWebUrl); //索引位置
        strTempText.remove(0,nIndex+strWebUrl.size()); //删除已遍历过的内容

    }
    return urlList;
}


QMenu* MainWindow::ShareRMenu(){

    return systemTrayIcon->m_menu;

}

int MainWindow::GetSlideSelRow(){

    //slideBar->GetSelectRow();
    return this->SlideSelItem;
}


void MainWindow::SearchChang( QString  text ) {

    qDebug() << "MainWindow::SearchChang " << text;


    for( int i = 0 ; i < downListView->m_dataModel->rowCount() ; i++  ){

        QStandardItemModel *mdb = downListView->m_dataModel;

        QString filename = mdb->index( i,0 ).data().toString();

        if( filename.indexOf(  text ) >= 0 ){

            downListView->selectRow( i );
        }
    }

}


//////////////////////////////////////////////////////////////////////////////////

void MainWindow::resizeEvent(QResizeEvent* event){
/**
    qDebug() <<"MMM "<<event->size();
    qDebug() <<"centerWidget " << centerWidget->size();
    qDebug() <<"downListView " << downListView->size();
**/
    int ww = downListView->size().width();
    downListView->SetTableWidth( ww );

    /** qDebug() <<"downListView " << downListView->size(); */
}

/**
*
*
=========================================================================*/
int MainWindow::initAria2cWork(){

    QProcess *ps  = new QProcess;
    QStringList options;
    options << "-c";
    //options << "ps aux | grep deepin_aria2c";
    options << "ps aux | grep aria2c";

    ps->start( "/bin/bash", options );
    ps->waitForFinished();

    QString strTemp = "";
    QStringList tmpList;
    strTemp = QString::fromLocal8Bit( ps->readAllStandardOutput() );
    tmpList.clear();
    tmpList = strTemp.split("\n");
    QString Z = "";
    foreach( QString str,tmpList ){

       if ( str == ""  ){
           continue;
       }
       if ( str.indexOf( "grep aria2c"  ) < 0 ){
           qDebug()<< str;
           Z = str;
           break;
       }else{
           continue;
       }
    }

    //qDebug()<< Z;
    int pid = 0;
    if ( Z != "" ){
        QStringList tmpArray = Z.split(" ");
        int i = 0;
        foreach ( QString tmp, tmpArray) {
            if ( tmp.trimmed() == "" ){
                continue;
            }
            i++;
            //qDebug() << "==>"<< i <<tmp.trimmed() ;
            if( i == 2 ){
                qDebug() << "============== deepin_aria2c PID ============>"<< tmp.trimmed() ;
                bool ok;
                pid = tmp.trimmed().toInt( &ok , 10 );
                break;
            }
        }
    }else{
        //qDebug() << "ariar2c 不在进程中...";
    }

    if ( pid != 0 ){

        qDebug() << "PID" << pid;

    }else{

        qDebug() << "ariar2c 不在进程中...";

        QString HomeDir = QDir::homePath();
        QString Downloads = HomeDir + "/Downloads";
        QString SessionFile = HomeDir + "/deepin_aria2c.session";
        QString SaveTime = "60";
        QString RPCPort = "19799";

        QProcess touch(0);
        QString cmd = "/usr/bin/touch";
        QStringList sessionfp;
        sessionfp.append( SessionFile );
        touch.start( cmd , sessionfp  );
        touch.waitForFinished();

        QString path;
        QDir dir;
        path = dir.currentPath();
        //aria2c --enable-rpc=true -c --disable-ipv6 --check-certificate=false --dir=/home/gaochong/Downloads/ --rpc-save-upload-metadata=true  --input-file=/home/gaochong/aria2c.session --save-session=/home/gaochong/aria2c.session --save-session-interval=60
        QProcess *aria2c = new QProcess;
        //QString command = path + "/deepin_aria2c";
        QString command = "/usr/bin/aria2c";

        QStringList args;
        /** 基本参数　*/
        args.append( "--dir="+ Downloads );
        args.append( "--input-file="+ SessionFile );
        args.append( "--save-session="+ SessionFile  );
        args.append( "--save-session-interval="+ SaveTime );

        /** RPC 参数　*/
        args.append( "--enable-rpc=true" );
        args.append( "--rpc-listen-port=" + RPCPort );  //rpc 通讯端口
        args.append( "--rpc-allow-origin-all=true");
        args.append( "--rpc-save-upload-metadata=true");

        /** 校验相关的参数　*/
        args.append( "--check-certificate=false");
        args.append( "--disable-ipv6");
        //args.append( "-c");

        ps->start( command, args );

        qDebug() << "发起进程" << ps->error() ;

     }


    //qDebug() << QDir::homePath();
    return pid;
}
















