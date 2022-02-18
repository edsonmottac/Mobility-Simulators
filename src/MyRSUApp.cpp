#include "MyRSUApp.h"
#include <string>
#include <iomanip>
#include <iostream>
#include <fstream>

#include <curl/curl.h>

using Veins::TraCIMobilityAccess;
using Veins::AnnotationManagerAccess;

using namespace veins_myproject;

Define_Module(MyRSUApp);

double somaVelocidades = 0;
double mediaVelocidades = 0;


#define NLIN 20
//#define NCOL 10
//double controlePista[NLIN][2];
int temID = 0;
int contaCarros = 0;
std::string situacao;
int cabecalhovetor = 0;

std::ofstream myfile;
int cabecalho = 0;
int contaBeacons = 0;

double previousTime= 0;
double elapsedTime= 0;
string alerts="";


//estrutura do veículo para manipulação no vetor
struct veiculo
  {
    std::string pista;
    double id;
    double velocidade;
    double posicaox;
    double posicaoy;
    double posroad;
  };

//vetor do tipo veículo estruturado
struct veiculo veiculosMapa[NLIN];


size_t writefunc2(void *ptr, size_t size, size_t nmemb, std::string *s)
{
  s->append(static_cast<char *>(ptr), size*nmemb);
  return size*nmemb;
}



void MyRSUApp::initialize(int stage) {
    //std::cout << "\n\n >>>> Inicializando RSU <<<<<<< \n\n";

    BaseWaveApplLayer::initialize(stage);
    if (stage == 0) {

        //conectando ao socket/FOG
        in2.Connect();


    }
}

//void MyRSUApp::receiveSignal(cComponent* source, simsignal_t signalID, cObject* obj, cObject* details) {
//}

void MyRSUApp::onWSA(WaveServiceAdvertisment* wsa) {
    std::cout << "\n PASSOU AQUI: onWSA "<<myId;
}

void MyRSUApp::onWSM(WaveShortMessage* wsm) {
    std::cout << "\n PASSOU AQUI: onWSM "<<myId;
}

void MyRSUApp::handleLowerMsg(cMessage* msg) {

    std::string c = msg->getClassName();
    //std::cerr << "\n\n\n TIPO: " << msg->getClassName() << "\n\n\n";

    if (c == "BasicSafetyMessage"){

        BasicSafetyMessage* bsm = check_and_cast<BasicSafetyMessage*>(msg);
        BeaconMessage* BC = dynamic_cast<BeaconMessage*>(bsm->decapsulate());
        contaBeacons++;

        std::ostringstream px;
        px << BC->getPosicaoX();

        std::ostringstream py;
        py << BC->getPosicaoY();

        std::ostringstream pp;
        pp << BC->getPosRoad();

        std::ostringstream lr;
        lr << BC->getLenRoad();

        std::ostringstream speed;
        speed << BC->getSpeed();

       //Passando querystring para o socket
       string    QS  =       "data-car;";
                 QS  = QS  + "id:"        +  BC->getIdSender()             + ";";
                 QS  = QS  + "veloact:"   +  speed.str()                   + ";";
                 QS  = QS  + "pista:"     +  BC->getRoadSender()           + ";";
                 QS  = QS  + "posx:"      +  px.str()                      + ";";
                 QS  = QS  + "posy:"      +  py.str()                      + ";";
                 QS  = QS  + "posroad:"   +  pp.str()                      + ";";
                 QS  = QS  + "lenroad:"   +  lr.str()                      + ";";
                 QS  = QS  + "simtime:"   +  simTime().str()               + ";\n";

       in2.SendMessage(QS);

       std::cerr         << "\n CHEGOU MENSAGEM NA RSU DE ->>  " <<  BC->getTypeDevice() << "\n ";


       // dados da mensagem enviada
       /*
        std::cerr        << "\n -------------------------------------------------- "
                         << "\n [" <<  BC->getTypeDevice() << "] X [RSU] : MENSAGEM RECEBIDA -------"
                         << "\n -- Recebida na RSU : "<< myId
                         << "\n -- Desencapsulando a Mensagem ----------- "
                         << "\n    -- Envida pelo veiculo: " << BC->getIdSender()
                         << "\n    -- Estrada : " << BC->getRoadSender()
                         << "\n    -- Velocidade : " << BC->getSpeed()
                         << "\n    -- POS X: " << BC->getPosicaoX()
                         << "\n    -- POS Y: " << BC->getPosicaoY()
                         << "\n    -- POS LR: " << BC->getLenRoad()
                         << "\n    -- POS PP: " << BC->getPosRoad()
                         << "\n -------------------------------------------------- ";

        */



       /*
        * Recupera dados da API Rest Python/Nuvem
        */
       CURL *curl;
       CURLcode res;
       std::string s;
       std::string url;


       if (previousTime==0){
           previousTime = simTime().dbl();
       }

       elapsedTime = (simTime().dbl() - previousTime);

       //std::cerr  << to_string(elapsedTime) << "\n";

       if (elapsedTime > 2.0){
           previousTime = simTime().dbl();

           std::cout << "Consutando  o serviço de alrtas...\n";

           curl = curl_easy_init();
           if(curl) {

              url = "http://127.0.0.1:5000/getAlert";

              curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
              curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc2);
              curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

              res = curl_easy_perform(curl);
              alerts = s;
              //std::cout << "\nAlertas: " << s << '\n';

          }
       }
    }
}

void MyRSUApp::handleSelfMsg(cMessage* msg) {

    switch (msg->getKind()) {
        case SEND_BEACON_EVT: {

            //Cria um objeto  BasicSafetyMessage
            BasicSafetyMessage* bsm = new BasicSafetyMessage();
            BeaconMessage* beaconMessage = new BeaconMessage("beacon");

            //Encapsula oObjetos em uma mensagem
            //beaconMessage->setRoadSender(mobility->getRoadId().c_str());
            beaconMessage->setIdSender("0");
            beaconMessage->setTypeDevice("RSU");
            beaconMessage->setAlerts(alerts.c_str());

            bsm->encapsulate(beaconMessage);

            populateWSM(bsm);
            sendDown(bsm);


            std::cerr       << "\n ------------------------------------- "
                            << "\n -- Mensagem enviada pela RSU: "<< myId
                            << "\n -- Alrta de Rotas: "<< alerts
                            << "\n ------------------------------------- \n";



            if (msg==sendBeaconEvt) {
                scheduleAt(simTime() + beaconInterval, sendBeaconEvt);
            }


            break;

        }
        case SEND_WSA_EVT:   {
            /*
            WaveServiceAdvertisment* wsa = new WaveServiceAdvertisment();
            populateWSM(wsa);
            sendDown(wsa);
            scheduleAt(simTime() + wsaInterval, sendWSAEvt);
            break;
            */

        }
        default: {
            if (msg)
                DBG_APP << "APP: Error: Got Self Message of unknown kind! Name: " << msg->getName() << endl;
            break;
        }
    }

}

void MyRSUApp::finish() {
    //cancelAndDelete(startAccidentMsg);
    cancelEvent(sendBeaconEvt);
}
