#include <stdlib.h>
#include "MyApplicaton.h"

#include <iostream>
#include <exception>

#include <jsoncpp/json/json.h>
#include <iostream>
#include <type_traits>
#include <tuple>

#include <list>
#include <iterator>

#include <curl/curl.h>

#include<string>
#include<iostream>


using Veins::TraCIMobilityAccess;
using Veins::AnnotationManagerAccess;
using Veins::TraCIConnection;

using Veins::TraCIColor;

using namespace veins_myproject;

#define black TraCIColor(0,0,0,255)
#define red TraCIColor(255,0,0,255)
#define green TraCIColor(0, 255, 0, 255)
#define yellow TraCIColor(255,255,0,255)
#define blue TraCIColor(0,0,255,255)
#define violet TraCIColor(255,0,255,255)
#define cyan TraCIColor(0,255,255,255)
#define white TraCIColor(255,255,255,255)

Define_Module(MyApplicaton);

double previousTime2= 0;
double elapsedTime2= 0;

std::map<int, std::string> veiculos_roteados;

struct Alerta {
    std::string Road;
    std::string Alerts;
};

int w = 0;
std::string idCar;

size_t writefunc3(void *ptr, size_t size, size_t nmemb, std::string *s)
{
  s->append(static_cast<char *>(ptr), size*nmemb);
  return size*nmemb;
}

std::string FindAndReplace(std::string fulltext, std::string find, std::string newtext) {
    size_t index = 0;
    while (true) {
         /* Locate the substring to replace. */
         index = fulltext.find(find, index);
         if (index == std::string::npos) break;

         /* Make the replacement. */
         fulltext = fulltext.replace(index, 1, newtext);

         /* Advance index forward so the next iteration doesn't pick it up as well. */
         index += 3;
    }
    return fulltext;

}

std::string getServiceRute(std::string id, std::string origem, std::string destino) {


    CURL *curl;
    CURLcode res;
    std::string s;
    std::string url;


    if (previousTime2==0){
        previousTime2 = simTime().dbl();
    }

    elapsedTime2 = (simTime().dbl() - previousTime2);

    //std::cerr  << to_string(elapsedTime) << "\n";

  //  if (elapsedTime2 > 2.0){
        previousTime2 = simTime().dbl();

        std::cout << "Consutando novas rotas disponpiveis...\n";

        curl = curl_easy_init();
        if(curl) {

            origem = FindAndReplace(origem,"#","_");
            destino= FindAndReplace(destino,"#","_");


           url = "http://127.0.0.1:5000/getRoute?vei=" + id + "&org=" + origem + "&dst=" + destino ;
           std::cout << url << '\n';


           curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
           curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc3);
           curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

           res = curl_easy_perform(curl);
           //std::cout << "\nGET-ROUTE: " << s << '\n';

           return s;

       }

   // }


}

/*
 *Este método lê os dados em json e retorna um a lista,  isso foi
 * feito para facilitar a manipulação deste conjunto de dados
 */
std::list<Alerta> JsonAlerts(std::string data) {

    std::list<Alerta> lista_trafego;

    lista_trafego={};

    std::string json_example = data;
    //std::cout << "\n" << json_example << "\n";

    Json::Value root;
    Json::Reader reader;
    bool parsedSuccess = reader.parse(json_example, root, false);

    //delete reader;

    if ( !parsedSuccess ) {
        std::cout << "Erro de Leitura dos Alertas de Tráfego" << endl;
    } else {

       const Json::Value dados = root["Alerts"];

       // Cria um objeto lista que ocnterá todos os alertas
       for (int i= 0; i < dados.size(); i++ ) {
             Alerta a;
             a.Road=dados[i]["Road"].asString();
             a.Alerts=dados[i]["Alerts"].asString();
             lista_trafego.push_back(a);
       }
    }
    // retorna a lista de alertas
    return lista_trafego;
}

std::list<std::string> JsonRoute(std::string data) {


      std::list<std::string> lista_rota;

      lista_rota={};

      std::string json_example = data;

      //{"Alerts": [{"Road": "8716807#6", "Ids": ";10;6;", "Alerts": "2"}]}

      //std::cout << "\n\nDADOS_RECEBIDOS " << json_example << "\n\n";


      Json::Value root;
      Json::Reader reader;
      bool parsedSuccess = reader.parse(json_example, root);

      //delete reader;

      if ( !parsedSuccess ) {
          std::cout << "Erro de Leitura dos Rotas de Tráfego" << endl;
      } else {
          std::cout << "Nova rota capturada" << endl;
         const Json::Value dados = root["root"];

         std::cout << dados;
         for (int i= 0; i < dados.size(); i++ ) {
             //std::cout << "\n" << dados[i] << "\n";
             lista_rota.push_back(dados[i].asString());
         }
      }
      // retorna a lista de alertas
      return lista_rota;
  }

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
       std::stringstream ss(s);
       std::string item;
       while (std::getline(ss, item, delim)) {
       elems.push_back(item);
       }
       return elems;
   }

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}




void MyApplicaton::initialize(int stage) {

    //std::cout << "\n\n >>>> Inicializando Vehiculos <<<<<<< \n\n"<<myId;

    BaseWaveApplLayer::initialize(stage);
        if (stage == 0) {

            //setup veins pointers
            mobility = TraCIMobilityAccess().get(getParentModule());
            traci = mobility->getCommandInterface();
            traciVehicle = mobility->getVehicleCommandInterface();
            lastDroveAt = simTime();
            //traciVehicle->setLaneChangeMode(0);

            sentMessage = false;
            lastDroveAt = simTime();
            currentSubscribedServiceId = -1;

        }
        delaySignal = registerSignal("delay");


}

void MyApplicaton::receiveSignal(cComponent* source, simsignal_t signalID, cObject* obj, cObject* details) {

    Enter_Method_Silent();
    if (signalID == mobilityStateChangedSignal) {
        handlePositionUpdate(obj);
    }
}

void MyApplicaton::handlePositionUpdate(cObject* obj) {

    BaseWaveApplLayer::handlePositionUpdate(obj);

        // stopped for for at least 10s?
        if (mobility->getSpeed() < 1) {
            if (simTime() - lastDroveAt >= 10 && sentMessage == false) {
                findHost()->getDisplayString().updateWith("r=16,red");
                sentMessage = true;

                /*
                WaveShortMessage* wsm = new WaveShortMessage();
                populateWSM(wsm);
                wsm->setWsmData(mobility->getRoadId().c_str());

                //host is standing still due to crash
                if (dataOnSch) {
                    startService(Channels::SCH2, 42, "Traffic Information Service");
                    //started service and server advertising, schedule message to self to send later
                    scheduleAt(computeAsynchronousSendingTime(1,type_SCH),wsm);
                }
                else {
                    //send right away on CCH, because channel switching is disabled
                    sendDown(wsm);
                }
                */
            }
        }
}

void MyApplicaton::handleSelfMsg(cMessage* msg) {



    switch (msg->getKind()) {
        case SEND_BEACON_EVT: {


            // Método criado na classe TraCICommandInterface "getTraCIXY"
            Coord coordOmnet = mobility->getCurrentPosition();
            std::pair<double,double> coordTraCI = traci->getTraCIXY(mobility->getCurrentPosition());

            //Cria um objeto  BasicSafetyMessage
            BasicSafetyMessage* bsm = new BasicSafetyMessage();

            BeaconMessage* beaconMessage = new BeaconMessage("beacon");

            // Ontem o ID do veículo. Mesmo ID utilizado no SUMO
            std::string carroid = std::to_string(getParentModule()->getIndex()+1);

            //Encapsula oObjetos em uma mensagem
            //beaconMessage->setRoadSender(mobility->getLaneId().c_str());// indica  a faixa que o carro está
            beaconMessage->setRoadSender(traciVehicle->getRoadId().c_str());
            beaconMessage->setIdSender(carroid.c_str());
            beaconMessage->setTypeDevice("CAR");
            beaconMessage->setPosicaoX(coordTraCI.first);
            beaconMessage->setPosicaoY(coordTraCI.second);
            beaconMessage->setPosRoad(traciVehicle->getLanePosition()); // posição do veículo ao longo da via
            beaconMessage->setLenRoad(traci->lane(traciVehicle->getLaneId()).getLength()); // Tamanho da via completa
            beaconMessage->setSpeed(mobility->getSpeed());

            bsm->encapsulate(beaconMessage);


            // tmepo de cada via
            //double x;
            //x = traci->getDistance(0, 0, returnDrivingDistance);
            //x = traci->road("297047310#2").getCurrentTravelTime();
            //std::cout << "\n" << x << "\n";
            //https://sumo.dlr.de/docs/Simulation/Routing.html



            //popula a memsagem com os dados encapsulados
            populateWSM(bsm);

            //Envia para a infraestrutura
            sendDown(bsm);

              /*
            std::cout     << "\n ---------------------------------------------------- "
                          << "\n [CAR] :: ENVIANDO MENSAGEM --------------------------"
                          << "\n -- ID DO VEÍCULO : "<< getParentModule()->getIndex()
                          << "\n -----------------------------------------------------"
                          << "\n ****************CONTEÚDO DA MENSAGEM*****************"
                          << "\n Velocidade Atual : " << mobility->getSpeed()
                          << "\n POSIÇÃO X: " << coordTraCI.first
                          << "\n POSIÇÃO Y: " << coordTraCI.second
                          << "\n ESTRADA: " << traciVehicle->getLaneId()
                          << "\n POSIÇÃO NA FAIXA: " << traciVehicle->getLanePosition()
                          << "\n TAMANHO TOTAL DA FAIXA: " << traci->lane(traciVehicle->getLaneId()).getLength()
                          << "\n ---------------------------------------------------- " << "\n";

                */


            if (msg==sendBeaconEvt) {
                scheduleAt(simTime() + beaconInterval, sendBeaconEvt);
            }

            break;
        }
        case SEND_WSA_EVT:   {
            //WaveServiceAdvertisment* wsa = new WaveServiceAdvertisment();
            //populateWSM(wsa);
            //sendDown(wsa);
            //scheduleAt(simTime() + wsaInterval, sendWSAEvt);
            break;
        }
        default: {
            if (msg)
                DBG_APP << "APP: Error: Got Self Message of unknown kind! Name: " << msg->getName() << endl;
            break;
        }
    }
}

void MyApplicaton::onWSM(WaveShortMessage* wsm) {
    //std::cerr << "\n Chegou uma Wave Short Message em: onWSM";
   // BasicSafetyMessage* WC = dynamic_cast<BasicSafetyMessage*>(bsm->decapsulate());
}

std::string removeSpaces(std::string word) {
    std::string newWord;
    for (int i = 0; i < word.length(); i++) {
            if (word[i] != ' ') {
                newWord += word[i];
            }
    }
    return newWord;
}

void MyApplicaton::onBSM(BasicSafetyMessage* bsm) {

    emit(delaySignal, 10.2);

    BeaconMessage* BC = dynamic_cast<BeaconMessage*>(bsm->decapsulate());

    int velo_antiga = bsm->getSenderSpeed().x;


    std::string tipo = BC->getTypeDevice();
    if (tipo =="RSU"){
        std::cout     << "\n CHEGOU MENSAGEM NO CARRO DE ->>  " <<  BC->getTypeDevice() << "\n ";


        if (BC != NULL) {

           if (BC->getAlerts() != "") {
               //std::cout << "\nALERTA DE TRÁFEGO EMITIDO!\n";

               // identifica os Ids do Sumo e Veins
               std::string idveins = std::to_string(getParentModule()->getIndex());
               std::string idSumo = mobility->getExternalId();

               //Armazena a rota atual do veículo em uma lista
               std::vector<std::string> rota_atual;
               std::list<std::string>  edges = traciVehicle->getPlannedRoadIds();
               std::string route_temp;

               /* zera o peso de todas as rotas */
               for (std::string element : edges) {
                   traciVehicle->changeRoute(element, 0);
                   rota_atual.push_back(element);
                   route_temp = route_temp + " | " +  element;
               }

               // Armazena a origem e o destino para eventual roteamento
               std::string origem;
               std::string destino;

               origem = traciVehicle->getRoadId().c_str(); // rua atual
               destino = rota_atual[rota_atual.size()-1];


               //std::cout << "\nORIGEM" << origem << "\n";
               //std::cout << "\nDESTINO" << destino << "\n";
               //std::cout << "\n---CARRO [" << idSumo << "]-----";
               //std::cout << "\n---ROTA -> " << route_temp;
               //std::cout << "\n--------------\n";



                // Armazena os alertas recebidos pela RSU em uma lista
                std::string d = BC->getAlerts();
                std::list<Alerta> ListaAlertas = JsonAlerts(d);

                // flag para verficiar se o carro está na direção de um alerta
                bool IsCritical = false;

                // compara a rota atual do veiculo com os alertas se o veiculo estiver na rota de algum alerta marca a flag IsCritical = true
                for (std::list<Alerta>::iterator it = ListaAlertas.begin(); it != ListaAlertas.end(); ++it){


                    std::cout << "\nCARRO: " << idSumo;
                    std::cout << "\nORIGEM: " << origem;
                    std::cout << "ROTA: \n " << route_temp << "\n";
                    //std::cout << "\nRUA X ATERTAS: " << it->Road << " - " << it->Alerts << "\n";

                    //traci->vehicle(idSumo).setColor(red);


                    int idxAlerta=-1;
                    int idxAtual = -1;
                    for (int i=0;i<=rota_atual.size()-1;i++) {
                        //std::cout << "\nCOMPARANDO:  " << it->Road << " COM " << rota_atual[i] << "\n";

                        //Indice da rua atual no array
                        if (rota_atual[i] == origem) {
                            idxAtual=i;
                        }

                        //indice da rua de alerta
                        if (it->Road == rota_atual[i]) {
                            idxAlerta=i; // posição do alerta no array
                        }
                    }
                    std::cout << "POSIÇÃO DA RUA ATUAL: " <<  idxAtual << "\n";
                    std::cout << "POSIÇÃO DO ALERTA O ARRAY: " <<  idxAlerta << "\n";

                    std::size_t found = route_temp.find(it->Road);

                    if (found!=std::string::npos) {

                        //evita mandar como origem uma junção para o calculo de roteamento (o caracter : indic o que é junção)
                        if (traciVehicle->getRoadId().find(":") == std::string::npos){

                              if ((rota_atual[0] != it->Road) && (rota_atual[rota_atual.size()-1] != it->Road) && (traciVehicle->getRoadId() != it->Road)) { // se não é uma rota origem nem destino, está apto ao reroteamento

                                  if (idxAlerta !=-1) {// esta na rota?

                                      if (rota_atual.size()-1 >= idxAtual+1) { // existe próxima rua?
                                          if (it->Road == rota_atual[idxAtual+1]) { // or  it->Road == rota_atual[idxAtual+2]    a próxima rua é a do alerta?
                                              IsCritical= true;
                                              //std::cout << "RUA ATUAL" << traciVehicle->getRoadId();
                                              //std::cout << "\n\nO Veículo : [" << idSumo <<  "] será reroteado \n";
                                              //std::cout << "\n Alrta na via: " << it->Road << "\n\n";
                                              traci->vehicle(idSumo).setColor(cyan); // marca em vermelho carros aptos ao reroteamento
                                          }
                                      }


                                      if (rota_atual.size()-1 >= idxAtual+2) { // existe segunda rua?
                                          if (it->Road == rota_atual[idxAtual+2]) { // or  it->Road == rota_atual[idxAtual+2]    a próxima rua é a do alerta?
                                              IsCritical= true;
                                              //std::cout << "RUA ATUAL" << traciVehicle->getRoadId();
                                              //std::cout << "\n\nO Veículo : [" << idSumo <<  "] será reroteado \n";
                                              //std::cout << "\n Alrta na via: " << it->Road << "\n\n";
                                              traci->vehicle(idSumo).setColor(cyan); // marca em vermelho carros aptos ao reroteamento
                                          }
                                      }




                                      if (rota_atual.size()-1 >= idxAtual+3) { // existe terçeira rua?
                                          if (it->Road == rota_atual[idxAtual+3]) { // or  it->Road == rota_atual[idxAtual+2]    a próxima rua é a do alerta?
                                              IsCritical= true;
                                              //std::cout << "RUA ATUAL" << traciVehicle->getRoadId();
                                              //std::cout << "\n\nO Veículo : [" << idSumo <<  "] será reroteado \n";
                                              //std::cout << "\n Alrta na via: " << it->Road << "\n\n";
                                              traci->vehicle(idSumo).setColor(cyan); // marca em vermelho carros aptos ao reroteamento
                                          }
                                      }
                                  }
                              }
                        }
                    }
                }

                // se for critico, chama o serviço de rotas
                if (IsCritical) {

                    std::string myroute = getServiceRute(idSumo, origem, destino);

                    if (myroute !=""){

                        std::list<std::string> newroute = JsonRoute(myroute);

                        if (newroute.size() > 0) {

                            std::string firstStr = newroute.front();
                            std::cout << "\nROTEANDO O CARRO " << idSumo;
                            //std::cout << " INICIO DA NOVA ROTA " << firstStr << "\n";
                            std::cout << " RUA ATUAL " << traciVehicle->getRoadId() << "\n";

                            //traciVehicle->changeVehicleRoute(newroute); // tem um erro no 21, porque?


                           // if (traciVehicle->getRoadId() == firstStr) {
                            try {
                                traciVehicle->changeVehicleRoute(newroute);

                                //veiculos_roteados[std::stoi(idSumo)] = "add";

                            } catch (const std::exception& e) {
                                traciVehicle->changeVehicleRoute(edges);
                                std::cerr << "\n\n FALHA NO ROTEAENTO GERAL [" << idSumo << "]\n"
                                          << "Descrição da Falha:" << e.what() << "\n\n";

                              }
                            //}
                        }
                    }

                } else {
                    // não critico

                }
                // fim roteamento veiculo
            } else {

            }

            //}
        }
    }
}

void MyApplicaton::finish() {
    //cancelAndDelete(startAccidentMsg);
    cancelEvent(sendBeaconEvt);
}

