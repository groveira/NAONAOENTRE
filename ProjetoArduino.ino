#include <Wire.h>
#include "LCDIC2.h"
#include <Keypad.h>

// ==========================================
// Configurações da Matriz de Teclas 4x4
// ==========================================
const byte LINHAS = 4;
const byte COLUNAS = 4;

char matrizdeTeclas[LINHAS][COLUNAS] = {
  {'D','#','0','*'},
  {'C','9','8','7'},
  {'B','6','5','4'},
  {'A','3','2','1'}
};

// ==========================================
// CONFIGURAÇÕES DE HARDWARE
// ==========================================
LCDIC2 lcd(0x27, 16, 2); 

byte pinosLinhas[LINHAS] = {8, 9, 10, 11}; 
byte pinosColunas[COLUNAS] = {4, 5, 6, 7}; 
Keypad teclado = Keypad(makeKeymap(matrizdeTeclas), pinosLinhas, pinosColunas, LINHAS, COLUNAS);

const int pinoSensor = 12;
const int pinoLedVerde = 2;
const int pinoLedVermelho = 13;
const int pinoReleSonalarme = 3; 

// ==========================================
// VARIÁVEIS DE CONTROLE DO SISTEMA
// ==========================================
String senhaConfigurada = "";
String senhaDigitada = "";

// Variável para escutar o código de fábrica secretamente
String comandoFabrica = ""; 

int estadoAlarme = 0; 
// 0 = Criando a senha inicial (ou redefinindo)
// 1 = Desarmado (Aguardando # para iniciar, ou código de fábrica)
// 2 = Digitando a senha para armar o sistema
// 3 = Armado e Monitorando a porta
// 4 = Digitando a senha para desarmar pacificamente
// 5 = Disparado / Perigo

void setup() {
  Serial.begin(9600);

  lcd.begin();      
  lcd.setBacklight(1); 

  // Configuração dos Pinos
  pinMode(pinoSensor, INPUT_PULLUP); 
  pinMode(pinoLedVerde, OUTPUT);
  pinMode(pinoLedVermelho, OUTPUT);
  
  // Relés Ativo-Baixo: mandamos HIGH antes de definir como OUTPUT 
  digitalWrite(pinoReleSonalarme, HIGH); 
  pinMode(pinoReleSonalarme, OUTPUT);

  // Garante que os LEDs comecem desligados, sem brilho (LOW = desliga)
  digitalWrite(pinoLedVerde, LOW);
  digitalWrite(pinoLedVermelho, LOW);
  
  // Garante que o Relé comece desligado, sem som (Relé invertido, HIGH = desliga)
  digitalWrite(pinoReleSonalarme, HIGH);

  // PASSO 1: Pedir ao usuário para digitar a senha que quer
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Crie sua Senha:");
  lcd.setCursor(0, 1);
}

void loop() {
  char tecla = teclado.getKey();
  int estadoAtualSensor = digitalRead(pinoSensor);

  // =========================================================
  // BIP DO TECLADO (Barulho do Teclado)
  // =========================================================
  if (tecla) {
    if (estadoAlarme != 5) {
      digitalWrite(pinoReleSonalarme, LOW);  // Liga
      delay(40);                             // Bip curtinho (40 ms)
      digitalWrite(pinoReleSonalarme, HIGH); // Desliga
    }
  }

  // ---------------------------------------------------------
  // ESTADO 0: USUÁRIO DIGITANDO A SENHA DE CONFIGURAÇÃO NOVA
  // ---------------------------------------------------------
  if (estadoAlarme == 0) {
    if (tecla && tecla != '*' && tecla != '#') { 
      senhaConfigurada += tecla;
      lcd.print(tecla); 

      if (senhaConfigurada.length() == 4) { 
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Senha Salva!");
        delay(2000);
        estadoAlarme = 1; 
        mostrarTelaDesarmado();
      }
    }
  }

  // ---------------------------------------------------------
  // ESTADO 1: DESARMADO (Neutro)
  // ---------------------------------------------------------
  else if (estadoAlarme == 1) {
    if (tecla) {
      if (tecla == '#') {
        // Vai armar o sistema
        estadoAlarme = 2; 
        senhaDigitada = "";
        comandoFabrica = ""; // Limpa a memória do comando pra não dar bug depois
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Armar. Senha:");
        lcd.setCursor(0, 1);
      } 
      else {
        // NOVO: Se apertou outra coisa no neutro, vai guardando na variável secreta
        comandoFabrica += tecla;
        
        // Mantém apenas os últimos 6 botões apertados na memória
        if (comandoFabrica.length() > 6) {
          comandoFabrica = comandoFabrica.substring(comandoFabrica.length() - 6);
        }

        // Se o que ele digitou formou a senha de fábrica, libera a troca
        if (comandoFabrica == "ABCD12") {
          estadoAlarme = 0;           // Joga pro Estado 0 (Criar Senha)
          senhaConfigurada = "";      // Apaga a senha velha
          comandoFabrica = "";        // Zera o comando
          
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("NOVA SENHA:");
          lcd.setCursor(0, 1);
        }
      }
    }
  }

  // ---------------------------------------------------------
  // ESTADO 2: DIGITANDO A SENHA PARA ARMAR
  // ---------------------------------------------------------
  else if (estadoAlarme == 2) {
    if (tecla) {
      senhaDigitada += tecla;
      lcd.print("*"); 

      if (senhaDigitada.length() == 4) {
        if (senhaDigitada == senhaConfigurada) {
          estadoAlarme = 3;
          digitalWrite(pinoLedVerde, HIGH); 
          
          digitalWrite(pinoReleSonalarme, LOW);  
          delay(200); 
          digitalWrite(pinoReleSonalarme, HIGH); 

          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("ALARME LIGADO!");
          delay(2000);
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Monitorando...");
        } else {
          lcd.clear();
          lcd.print("Senha Incorreta");
          delay(2000);
          estadoAlarme = 1; 
          mostrarTelaDesarmado();
        }
      }
    }
  }

  // ---------------------------------------------------------
  // ESTADO 3: ARMADO, MONITORANDO A PORTA E ESPERANDO O USUÁRIO TECLAR A #
  // ---------------------------------------------------------
  else if (estadoAlarme == 3) {
    
    if (estadoAtualSensor == HIGH) {
      estadoAlarme = 5; 
      digitalWrite(pinoLedVerde, LOW);     
      digitalWrite(pinoLedVermelho, HIGH); 
      
      digitalWrite(pinoReleSonalarme, LOW); // Fica ligado!        

      senhaDigitada = ""; 
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Perigo, Senha:");
      lcd.setCursor(0, 1);
    }
    
    else if (tecla == '#') {
      estadoAlarme = 4;
      senhaDigitada = "";
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Desarmar. Senha:");
      lcd.setCursor(0, 1);
    }
  }

  // ---------------------------------------------------------
  // ESTADO 4: DIGITANDO A SENHA PARA DESARMAR (SEM PERIGO)
  // ---------------------------------------------------------
  else if (estadoAlarme == 4) {
    if (tecla) {
      senhaDigitada += tecla;
      lcd.print("*");

      if (senhaDigitada.length() == 4) {
        if (senhaDigitada == senhaConfigurada) {
          estadoAlarme = 1;
          digitalWrite(pinoLedVerde, LOW); 
          
          digitalWrite(pinoReleSonalarme, LOW);  
          delay(150);
          digitalWrite(pinoReleSonalarme, HIGH); 
          delay(150);
          digitalWrite(pinoReleSonalarme, LOW);  
          delay(150);
          digitalWrite(pinoReleSonalarme, HIGH); 

          lcd.clear();
          lcd.print("Alarme Desligado");
          delay(2000);
          mostrarTelaDesarmado();
        } else {
          lcd.clear();
          lcd.print("Senha Incorreta");
          delay(2000);
          estadoAlarme = 3; 
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Monitorando...");
        }
      }
    }
  }

  // ---------------------------------------------------------
  // ESTADO 5: DISPARADO / PERIGO TOTAL
  // ---------------------------------------------------------
  else if (estadoAlarme == 5) {
    
    if (tecla) {
      senhaDigitada += tecla;
      lcd.print(tecla); 

      if (senhaDigitada.length() == 4) {
        if (senhaDigitada == senhaConfigurada) {
          estadoAlarme = 1;
          digitalWrite(pinoLedVermelho, LOW); 
          
          digitalWrite(pinoReleSonalarme, HIGH); // Cala o Sonalarme
          
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Alarme Neutro");
          delay(2000);
          mostrarTelaDesarmado();
        } else {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Incorreta!!!");
          delay(1000);
          
          senhaDigitada = ""; 
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Perigo, Senha:");
          lcd.setCursor(0, 1);
        }
      }
    }
  }
}

// ==========================================
// FUNÇÕES AUXILIARES DO CÓDIGO
// ==========================================
void mostrarTelaDesarmado() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sistema Desativo");
  lcd.setCursor(0, 1);
  lcd.print("Aperte # p/ ligar");
}