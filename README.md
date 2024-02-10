# storage-service
key-value pair storage service

# Sobre a Aplicação
O objetivo geral do projeto foi concretizar um serviço de armazenamento de pares chave- valor (nos moldes da interface java.util.Map da API Java) similar ao utilizado pela Amazon para dar suporte aos seus serviços Web. Neste sentido, a estrutura de dados utilizada para armazenar esta informação é uma árvore de pesquisa binária, dada a sua elevada eficiência ao nível da pesquisa.

foram definidas estruturas de dados e implementadas várias funções para lidar com a manipulação dos dados que vão ser armazenados na árvore binária, bem como para gerir uma árvore local que suporte um subconjunto dos serviços definidos pela interface Map.

Tem as funções necessárias para serializar e de-serializar estruturas complexas usando Protocol Buffer, um servidor concretizando a árvore binária, e um cliente com uma interface de gestão do conteúdo da árvore binária

foi criado um sistema concorrente que aceita e processa pedidos de múltiplos clientes em simultâneo através do uso de multiplexagem de I/O e múltiplas threads.

A aplicação suporta tolerância a falhas através de replicação do estado do servidor, seguindo o modelo Chain Replication (Replicação em Cadeia)  e usando o serviço de coordenação ZooKeeper.

# About
The general objective of the project was to implement a key-value pair storage service (along the lines of the Java API's java.util.Map interface) similar to that used by Amazon to support its Web services. In this sense, the data structure used to store this information is a binary search tree , given its high efficiency at the search level.

data structures were defined and several functions were implemented to handle the manipulation of data that will be stored in the binary tree, as well as to manage a local tree that supports a subset of the services defined by the Map interface.

It has the necessary functions to serialize and de-serialize complex structures using Protocol Buffer, a server implementing the binary tree, and a client with an interface for managing the contents of the binary tree

A concurrent system was created that accepts and processes requests from multiple clients simultaneously through the use of I/O multiplexing and multiple threads.

The application supports fault tolerance through server state replication, following the Chain Replication model and using the ZooKeeper coordination service.

# Tecnologias utilizadas
## Back end
- Linguagem C



# Como executar o projeto



# Autor

Erickson Cacondo

https://www.linkedin.com/in/erickson-cacondo-8a504b1a4?utm_source=share&utm_campaign=share_via&utm_content=profile&utm_medium=ios_app

