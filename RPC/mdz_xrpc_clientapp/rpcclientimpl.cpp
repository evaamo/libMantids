#include "rpcclientimpl.h"
#include "globals.h"

#include <thread>

#include <mdz_net_sockets/cryptostream.h>

using namespace Mantids::RPC;
using namespace Mantids::Application;

RPCClientImpl::RPCClientImpl()
{
    getClientConfigCmd="getClientConfig";
    updateClientConfigLoadTimeCmd="updateClientConfigLoadTime";
    failedToRetrieveC2Config = false;
}

RPCClientImpl::~RPCClientImpl()
{

}

void RPCClientImpl::runRPClient0(RPCClientImpl *rpcImpl)
{
    rpcImpl->runRPClient();
}

void RPCClientImpl::runRPClient()
{
    uint16_t remotePort = Globals::getLC_C2RemotePort();
    std::string remoteAddr = Globals::getLC_C2RemoteAddress();
    int secsBetweenConnections = Globals::getLC_C2TimeBetweenConnections();

    addMethods();

    for (;;)
    {
        Mantids::Network::TLS::Socket_TLS sockRPCClient;

        std::string caCertPath = Globals::getLC_TLSCAFilePath();
        std::string privKeyPath = Globals::getLC_TLSKeyFilePath();
        std::string pubCertPath = Globals::getLC_TLSCertFilePath();

        if (!sockRPCClient.setTLSCertificateAuthorityPath(  caCertPath.c_str() ))
        {
            Globals::getAppLog()->log0(__func__,Logs::LEVEL_ERR, "Error starting RPC Connector to %s:%d: Bad/Unaccesible TLS Certificate Authority (%s)", remoteAddr.c_str(), remotePort, caCertPath.c_str());
            _exit(-3);
        }
        if (!sockRPCClient.setTLSPrivateKeyPath(  privKeyPath.c_str() ))
        {
            Globals::getAppLog()->log0(__func__,Logs::LEVEL_ERR, "Error starting RPC Connector to %s:%d: Bad/Unaccesible TLS Private Certificate (%s)", remoteAddr.c_str(), remotePort, privKeyPath.c_str());
            _exit(-3);
        }
        if (!sockRPCClient.setTLSPublicKeyPath(  pubCertPath.c_str() ))
        {
            Globals::getAppLog()->log0(__func__,Logs::LEVEL_ERR, "Error starting RPC Connector to %s:%d: Bad/Unaccesible TLS Public Certificate (%s)", remoteAddr.c_str(), remotePort, pubCertPath.c_str());
            _exit(-3);
        }

        Globals::getAppLog()->log0(__func__,Logs::LEVEL_INFO,  "Connecting to RPC Server %s:%d...", remoteAddr.c_str(), remotePort);

        if ( sockRPCClient.connectTo( remoteAddr.c_str(), remotePort ) )
        {
            Globals::getAppLog()->log0(__func__,Logs::LEVEL_INFO,  "RPC Client Connected to server %s:%d (CN=%s)", remoteAddr.c_str(), remotePort, sockRPCClient.getTLSPeerCN().c_str());

            if (postConnect(&sockRPCClient))
            {
                Mantids::Network::Streams::CryptoStream cstreams(&sockRPCClient);
                if (cstreams.mutualChallengeResponseSHA256Auth(Globals::getLC_C2NetresApiKey(),false) == std::make_pair(true,true))
                {
                    // now is fully connected / authenticated...
                    if (failedToRetrieveC2Config)
                        connectedToC2AfterFailingToLoadC2Config();
                    fastRPC.processConnection(&sockRPCClient,"SERVER");
                }
                else
                {
                    Globals::getAppLog()->log0(__func__,Logs::LEVEL_ERR, "Invalid API Key @RPC connector to %s:%d", remoteAddr.c_str(), remotePort);
                }
            }
            Globals::getAppLog()->log0(__func__,Logs::LEVEL_WARN,  "RPC Client disconnected from %s:%d (CN=%s)", remoteAddr.c_str(), remotePort, sockRPCClient.getTLSPeerCN().c_str());
        }
        else
        {
            Globals::getAppLog()->log0(__func__,Logs::LEVEL_ERR, "Error connecting to remote RPC Server @%s:%d: %s", remoteAddr.c_str(), remotePort, sockRPCClient.getLastError().c_str());
        }

        sleep(secsBetweenConnections);
    }
}

bool RPCClientImpl::retrieveConfigFromLocalFile()
{
    /////////////////////////////////////////////////////////////
    //***********CONFIG RETRIEVE FROM FILE**********
    /////////////////////////////////////////////////////////////
    std::string localConfigPath = Globals::getLC_RemoteConfigFilePath();
    Globals::getAppLog()->log0(__func__,Logs::LEVEL_INFO, "Retrieving config from local file: %s",localConfigPath.c_str());

    std::ifstream infile(localConfigPath);

    if (infile.is_open())
    {
        std::string lineInFile;
        if (std::getline(infile, lineInFile))
        {
            std::string sRemoteConfigDecrypted = decryptStr(lineInFile);

            json jRemoteConfig;
            Mantids::Helpers::JSONReader2 reader;
            bool parsingSuccessful = reader.parse( sRemoteConfigDecrypted, jRemoteConfig );
            if ( parsingSuccessful )
            {
                jRetrievedConfig=jRemoteConfig;
                infile.close();
                return true;
            }
        }
        infile.close();
    }
    return false;
}

bool RPCClientImpl::retrieveConfigFromC2()
{
    /////////////////////////////////////////////////////////////
    //***********INITIALIZE C2 COMM AND CONFIG RETRIEVE**********
    /////////////////////////////////////////////////////////////
    json rpcError;

    failedToRetrieveC2Config = false;

    Globals::getAppLog()->log0(__func__,Logs::LEVEL_INFO, "Retrieving config from remote C2.");

    // Try to retrieve the configuration from the C&C. (will try several attempts)
    json jRemoteConfig = fastRPC.runRemoteRPCMethod("SERVER",getClientConfigCmd,{},&rpcError);
    if (rpcError["succeed"].asBool() == true)
    {
        // Translate this config to the configuration file...
        std::string sRemoteConfig = jRemoteConfig.toStyledString();
        std::string sLocalConfig = jRetrievedConfig.toStyledString();

        if (sRemoteConfig == sLocalConfig)
        {
            Globals::getAppLog()->log0(__func__,Logs::LEVEL_INFO, "C2 remote/local configuration is the same. Not upgrading.");
            return true;
        }

        if ( sRemoteConfig.size()>7 )
        {
            std::string sRemoteConfigEncrypted = encryptStr(sRemoteConfig);

            std::string localConfigPath = Globals::getLC_RemoteConfigFilePath();


            std::ofstream outfile;
            outfile.open(localConfigPath, std::ios_base::out);
            if (outfile.is_open())
            {
                outfile  << sRemoteConfigEncrypted << "\n";
                outfile.close();

                jRetrievedConfig=jRemoteConfig;

                json ans;
                ans["x"] = fastRPC.runRemoteRPCMethod("SERVER",updateClientConfigLoadTimeCmd,{},&rpcError);

                if ( rpcError["succeed"].asBool() == false )
                {
                    Globals::getAppLog()->log0(__func__,Logs::LEVEL_ERR, "Configuration loaded from the remote server, but failed to update the C2 config access time... %s", rpcError["errorMessage"].asCString());
                }

                if ( JSON_ASBOOL(ans,"x",false)==false )
                {
                    Globals::getAppLog()->log0(__func__,Logs::LEVEL_ERR, "Configuration loaded from the remote server, but failed to update the C2 config access time.");
                }

                Globals::getAppLog()->log0(__func__,Logs::LEVEL_INFO, "C2 configuration written to: %s",localConfigPath.c_str());

                return true;
            }
            else
            {
                Globals::getAppLog()->log0(__func__,Logs::LEVEL_ERR, "Failed to write the remote configuration to: %s", localConfigPath.c_str());
            }
        }
        else
        {
            Globals::getAppLog()->log0(__func__,Logs::LEVEL_ERR, "Remote configuration from the C2 is not reliable.");
        }
    }
    else
    {
        Globals::getAppLog()->log0(__func__,Logs::LEVEL_ERR, "Can't retrieve configuration from the C2: %s", rpcError["errorMessage"].asCString());
    }

    // If the C2 is available in the near future, handle it (recommendation: exit the program).
    failedToRetrieveC2Config = true;


    return false;
}

json RPCClientImpl::getJRetrievedConfig()
{
    return jRetrievedConfig;
}
