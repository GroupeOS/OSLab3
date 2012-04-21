/****************************************************************
 * Lab3.cpp
 *
 * François Potvin Naud,
 * Michaël Guérette,
 * Mathieu Lapierre,
 * Gabriel Le Breton
 *
 * Dans le cadre du cours
 * 8INF341 - Systèmes d'exploitation
 ****************************************************************/

#include <fstream>
#include <vector>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <list>
#include <stdlib.h>
#include <sys/timeb.h>
#include <string>
#include <stdint.h> // Pour les uint8_t

#define QTE_INSTRUCTIONS 2000000 //Max de cette valeur: Valeur maximum d'un int (4294967294 max pour un unsigned int)
#define MAX_FILESIZE 128 //8 fichiers dans 1024

using namespace std;

/****************************************************************
 * Variables globales et structures
 ****************************************************************/

struct file
{
    char nom[6];
    uint16_t taille;
    uint8_t premierBloc;
};

/****************************************************************
 * Headers
 ****************************************************************/

// Le disque dur
class HardDrive
{
public:
    HardDrive();
    ~HardDrive();
	void readBlock(uint8_t NumeroBlock, char*& TampLecture);
	void writeBlock (uint8_t NumeroBlock, char* TampEcriture);
	
    file readfile (uint8_t NumeroFicher);
	void writefile(uint8_t NumeroFicher, file fichier);
	
	void readFAT(uint8_t NumeroBlock, uint8_t*& TampLecture);
	void writeFAT (uint8_t NumeroBlock, uint8_t TampEcriture);
	
private:
    void Preparefile();
	void Deletefile();
	
	void* readInfile(int pos, int lenght);
	void writeInfile(int pos,int lenght, void* data);
};

class OS
{
public:
	OS();
	~OS();
	void Run();
	void read(char nomFichier[6], int pos, int nbCaracteres, string tampLecture);
	void write(char nomFichier[6], int pos, int nbCaracteres, string TampEcriture);
	void deleteEOF(char nomFichier[6], int pos);
	
private:
	void ManageFile();
	void AfficherStat();
	void AfficherHardDrive();
	
	file lireFichier(int pos);
	void ecrireFichier(int pos, file fichier);
	
	uint8_t lireFat(uint8_t pos);
	void ecrireFat(uint8_t pos, uint8_t val);
	
	void lireBloc(uint8_t pos, char* output);
	void ecrireBloc(uint8_t pos, char data[4]);
	
	file trouverFichier(char nomFichier[6], bool create = false, int *pos = NULL);
	
	void creerFichierBlocVide(bool force);
	int blocSuivant(uint8_t bloc);
	void freeBloc(int bloc);
	int newBloc();
	
    HardDrive HD;
    int currInstruction;
    unsigned short runningtime, LastTime;
    struct timeb tp;
    vector<file> fileList;
};

/****************************************************************
 * Implementations de HardDrive
 ****************************************************************/


HardDrive::HardDrive()
{
	Preparefile();
}

HardDrive::~HardDrive()
{
	//Deletefile();
}

void HardDrive::readBlock(uint8_t NumeroBlock, char*& TampLecture)
{
	//if(NumeroBlock <256)
	{
		TampLecture = (char*)readInfile(NumeroBlock*4 + 90 + 256,4);
	}
}

void HardDrive::writeBlock(uint8_t NumeroBlock, char* TampEcriture)
{
	//if(NumeroBlock <256)
	{
		writeInfile(NumeroBlock*4 + 90 + 256,4, TampEcriture);
	}
}

file HardDrive::readfile(uint8_t NumeroFicher)
{
	if(NumeroFicher < 10)
	{
		file tempFile;
		char* TampLecture;
		TampLecture = (char*)readInfile(NumeroFicher*9,9);
		memcpy(&tempFile,TampLecture,sizeof(file));
		return tempFile;
	}
	else
	{
		// Ne devrait jamais etre atteint
		exit(-1);
	}
}

void HardDrive::writefile(uint8_t NumeroFicher, file fichier)
{
	if(NumeroFicher < 10)
	{
		writeInfile(NumeroFicher * 9, 6, fichier.nom);
		writeInfile(NumeroFicher * 9 + 6, 2, &fichier.taille);
		writeInfile(NumeroFicher * 9 + 8, 1, &fichier.premierBloc);
	}
}

void HardDrive::readFAT(uint8_t NumeroBlock, uint8_t*& TampLecture)
{
	//if(NumeroBlock <256)
	{
		TampLecture = (uint8_t*)readInfile(NumeroBlock + 90,1);
	}
}

void HardDrive::writeFAT (uint8_t NumeroBlock, uint8_t TampEcriture)
{
	//if(NumeroBlock <256)
	{
		writeInfile(NumeroBlock + 90,1, &TampEcriture);
	}
}

void HardDrive::Preparefile()
{
	char data= '0';
	ofstream fileOut("HD.DH");
	for(int i=0;i<(90+256+1024);i++)
	{
		fileOut.write(&data, sizeof(char));
	}
	fileOut.close();
}

void HardDrive::Deletefile()
{
	remove("HD.DH");
}

void* HardDrive::readInfile(int pos, int lenght)
{
	void* buffer;
	buffer = new char[lenght];
	ifstream fileIn("HD.DH", ios::binary);
	if(fileIn != NULL)
	{
		fileIn.seekg(pos,ios_base::beg);
		fileIn.read((char*)buffer, lenght);
		fileIn.close();
	}
	return buffer;
}

void HardDrive::writeInfile(int pos,int lenght, void* data)
{
	ofstream fileOut("HD.DH",ios::out | ios::in | ios::binary);
	fileOut << dec;
	if(fileOut != NULL)
	{
		fileOut.seekp(pos,ios_base::beg);
		fileOut.write((char*)data, lenght);
		fileOut.close();
	}
}


/****************************************************************
 * Implementations de l'OS
 ****************************************************************/


OS::OS()
{
	creerFichierBlocVide(true);
	currInstruction = 1;
	runningtime = 0;
	LastTime = 0;
	ftime(&tp);
	LastTime = tp.millitm;
}
OS::~OS()
{
}

void OS::Run()
{
	while(currInstruction < QTE_INSTRUCTIONS)
	{
		ManageFile();
		AfficherStat();
		currInstruction++;
	}
}

void OS::read(char nomFichier[6], int pos, int nbCaracteres, string tampLecture)
{
	file fichier = trouverFichier(nomFichier);
	if (strncmp(fichier.nom, "000000", 6) == 0)
		return;
	if (fichier.taille < pos+nbCaracteres)
		return;
	
	int bloc = fichier.premierBloc;
	for (int i = 0; i < pos/4; i++)
	{
		bloc = blocSuivant(bloc);
	}
	for (int i = pos/4; i <= (pos+nbCaracteres)/4; i++)
	{
		char data[4];
		
		lireBloc(bloc, data);
		
		tampLecture.append(data, 4);
		
		bloc = blocSuivant(bloc);
	}
	int firstCaractToRemove = pos - ((pos/4)*4);
	tampLecture = tampLecture.substr(firstCaractToRemove, nbCaracteres);
}

void OS::write(char nomFichier[6], int pos, int nbCaracteres, string TampEcriture)
{
	int var;
	file fichier = trouverFichier(nomFichier, true, &var);
	if (strncmp(fichier.nom, "000000", 6)==0)
		return;
	
	if (fichier.taille < pos+nbCaracteres)
	{
		int dernierBloc = fichier.taille/4;
		int nouveauDernierBloc = (pos+nbCaracteres)/4;
		int blocAjouter = nouveauDernierBloc - dernierBloc;
		int bloc = fichier.premierBloc;
		int lastBloc;
		while (bloc != -1)
		{
			lastBloc = bloc;
			bloc = blocSuivant(bloc);
		}
		for (int i=0; i < blocAjouter; i++)
		{
			int nouveauBlocLibre = newBloc();
			if (nouveauBlocLibre == -1)
				return;
			
			ecrireFat(lastBloc, nouveauBlocLibre);
			lastBloc = blocSuivant(lastBloc);
		}
		fichier.taille = pos+nbCaracteres;
		ecrireFichier(var, fichier);
	}
	int bloc = fichier.premierBloc;
	
	for (int i = 0; i < pos/4; i++)
	{
		bloc = blocSuivant(bloc);
	}
	for (int i = pos/4; i <= (pos+nbCaracteres)/4; i++)
	{
		char data[4];
		
		if (i*4 < pos)
		{
			lireBloc(bloc, data);
			for (int j = pos%4; j < 4; j++)
			{
				data[j] = TampEcriture[0];
				if (TampEcriture.length() >= 1)
					TampEcriture = TampEcriture.substr(1, TampEcriture.length());
			}
			ecrireBloc(bloc, data);
		}
		else if (i*4 > pos+nbCaracteres)
		{
			lireBloc(bloc, data);
			for (int j = 0; j < pos+nbCaracteres%4; j++)
			{
				data[j] = TampEcriture[0];
				if (TampEcriture.length() >= 1)
					TampEcriture = TampEcriture.substr(1, TampEcriture.length());
			}
			ecrireBloc(bloc, data);
		}
		else
		{
			for (int j = 0; j < 4; j++)
			{
				data[j] = TampEcriture[0];
				if (TampEcriture.length() >= 1)
					TampEcriture = TampEcriture.substr(1, TampEcriture.length());
			}
			ecrireBloc(bloc, data);
		}			
		bloc = blocSuivant(bloc);
	}
}

void OS::deleteEOF(char nomFichier[6], int pos)
{
	file fichier = trouverFichier(nomFichier);
	if (strncmp(fichier.nom, "000000", 6) == 0)
		return;
	if (fichier.taille < pos)
		return;
	fichier.taille = pos;
	int bloc = fichier.premierBloc;
	for(int i=0; i <= pos/4; i++)
	{
		bloc = blocSuivant(bloc);
	}
	int prochainBloc = blocSuivant(bloc);
	while (bloc != -1)
	{
		ecrireFat(bloc, bloc);
		freeBloc(bloc);
		bloc = prochainBloc;
		prochainBloc = blocSuivant(bloc);
	}
}

void OS::ManageFile()
{
	int pos, nbChar;
	char randChar;
	randChar = 97 + rand() % 8;
	pos = rand() % MAX_FILESIZE;
	vector<file>::iterator itFile;
	
	char fileName[6] = " .txt";
	fileName[0] = randChar;
	
	bool exist = false;
	itFile = fileList.begin();
	while(itFile != fileList.end() && !exist)
	{
		if(strncmp(itFile->nom,fileName, 6) == 0) //Return 0 si les strings sont pareils
		{
			exist = true;
		}
		itFile++;
	}
	
	char* buffer;
	
	int action = rand() % 3;
	if(!exist)
		action = 0;
	switch (action)
	{
			// Ecriture d'un fichier
		case 0:
		{
			nbChar = rand() % (MAX_FILESIZE - pos);
			buffer = new char[nbChar];
			file tmpFile;
			for(int i=0; i<6; i++)
			{
				tmpFile.nom[i] = fileName[i];
			}
			
			for(int i = 0; i<nbChar; i++)
			{
				buffer[i] = randChar;
			}
			write(fileName, pos, nbChar, buffer);
			if(!exist)
			{
				fileList.push_back(tmpFile);
			}
			break;
		}
			// Suppresion de la fin d'un fichier
		case 1:
		{
			deleteEOF(fileName, pos);
			if(pos == 0)
			{
				fileList.erase(itFile);
			}
			break;
		}
			// Suppression d'un fichier
		case 2:
		{
			deleteEOF(fileName, 0);
			fileList.erase(itFile);
			break;
		}
		default:
			break;
	}
}

void OS::AfficherStat()
{
	ftime(&tp);
	runningtime = (tp.millitm-LastTime);
	
	if (runningtime >= 5000)
	{
		//system("clear");
		
		//cout << "Affiche" <<  runningtime << endl;
		
		AfficherHardDrive();
		runningtime = 0;
		ftime(&tp);
		LastTime = tp.millitm;
	}
}

void OS::AfficherHardDrive()
{   	
	char* buffer = new char[4];
	
	system("clear");
	
	int k = 0;
	for (int i = 1; i <= 32; i++)
	{
		for (int j = 1; j <= 8; j++)
		{
			lireBloc(k, buffer);
			cout << "0x";
			cout << setfill ('0') << setw (2);
			cout << hex << k; //hex traduit les int en hexadecimal sur 3 charactère
			cout  << " " << buffer << "  ";
			
			k++;
		}
		cout << "\n";
	}
	k = 0;
	
	vector<file> HDList;
	
	// Parcourt de la liste des 8 fichiers
	for (int i = 1; i <= 8; i++)
	{
		HDList.push_back(lireFichier(i));
	}
	
	vector<file>::iterator itFile;
	
	cout << "Liste des fichiers" << endl;
	int i = 0;
	
	for(itFile = HDList.begin(); itFile != HDList.end(); itFile++)
	{
		i++;
		int premierBloc = itFile->premierBloc;
		int dernierBloc = premierBloc + itFile->taille;
		cout << itFile->nom << " ";
		cout << hex << "0x" << premierBloc << " ";
		cout << hex << "0x" << dernierBloc << endl;
	}
	
	cout << endl;
}

file OS::lireFichier(int pos) //pos = # de fichier (1 à 8)
{
	return HD.readfile(pos);
}

void OS::ecrireFichier(int pos, file fichier)
{
	HD.writefile(pos, fichier);
}

uint8_t OS::lireFat(uint8_t pos)
{
	uint8_t retour;
	uint8_t *ptr;
	HD.readFAT(pos, ptr);
	retour = (*ptr);
	delete ptr;
	return retour;
}

void OS::ecrireFat(uint8_t pos, uint8_t val)
{
	HD.writeFAT(pos, val);
}

void OS::lireBloc(uint8_t pos, char* output)
{
	HD.readBlock(pos, output);
}

void OS::ecrireBloc(uint8_t pos, char data[4])
{
	HD.writeBlock(pos, data);
}

file OS::trouverFichier(char nomFichier[6], bool create, int *pos)
{
	for(int i=1; i < 10; i++)
	{
		file fichier;
		fichier = lireFichier(i);
		if(strncmp(nomFichier, fichier.nom, 6) == 0)
		{
			if (pos != NULL)
			{
				(*pos) = i;
			}
			return fichier;
		}
	}
	if(create)
	{
		for(int i=1; i<10; i++)
		{
			file fichier;
			fichier = lireFichier(i);
			if(strncmp("000000", fichier.nom, 6) == 0)
			{
				strcpy(fichier.nom, nomFichier);
				fichier.taille = 0;	//taille 0: aucun bloc d'alloué
				fichier.premierBloc = newBloc();
				
				ecrireFichier(i, fichier);
				if (pos != NULL)
				{
					pos = new int;
					(*pos) = i;
				}
				return fichier;
			}
		}
	}
	file vide;
	strcpy(vide.nom,  "000000");
	vide.taille = 0;
	vide.premierBloc = 0;
	if (pos != NULL)
	{
		pos = new int;
		(*pos) = -1;
	}
	return vide;
}

void OS::creerFichierBlocVide(bool force)
{
	if (!force)
	{
		file fichier = lireFichier(0);
		if (strcmp(fichier.nom, "free.")!=0)
			return;
	}   
	
	file free;      //fichier de bloc libre
	strcpy(free.nom,"free.");
	free.taille = 1024;
	free.premierBloc = 0;
	for(int i = 0; i < 256; i++)
	{
		if (i == 255)
			ecrireFat(i, i);    //le dernier bloc d'un fichier se reconnais car il pointe sur lui-même
		else
			ecrireFat(i, i+1);
	}
}

int OS::blocSuivant(uint8_t bloc)
{
	int retour = lireFat(bloc);
	if (retour == bloc)
		return -1;
	return retour;
}

void OS::freeBloc(int bloc)
{
	file free = lireFichier(0);
	ecrireFat(bloc, free.premierBloc);
	free.premierBloc = bloc;
	free.taille += 4;
	ecrireFichier(0, free);
}
	
int OS::newBloc()
{
	file free = lireFichier(0);
	if (free.taille == 0)
		return -1;
	
	int bloc = free.premierBloc;
	int prochainBlocLibre= lireFat(free.premierBloc);
	ecrireFat(bloc,bloc);
	free.premierBloc = prochainBlocLibre;
	free.taille -= 4;
	ecrireFichier(0, free);
	return bloc;
}


/****************************************************************
 * Exécution principale
 ****************************************************************/

int main()
{
    OS PatOS;
    
    cout << "ON" << endl;
    for(int i = 0; i<8; i++)
    {
        PatOS.Run();
    }
    cout << "OFF" << endl;
    return 0;
}
