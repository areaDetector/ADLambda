#include "LambdaDataRecvFactory.h"
#include "LambdaDataReceiver.h"

namespace DetLambdaNS
{
    uptr_datarecv LambdaDataRecvFactory::CreateDataReceiver(Enum_readout_mode& mode)
    {
        uptr_datarecv data_recv;
        switch(mode)
        {
            case OPERATION_MODE_12:
                data_recv = uptr_datarecv(new LambdaDataReceiver12BitMode());
                break;
            case OPERATION_MODE_24:
                data_recv = uptr_datarecv(new LambdaDataReceiver24BitMode());
                break;
            case OPERATION_MODE_2x12:
                /// TODO
                break;
            case OPERATION_MODE_UNKNOWN:
                /// TODO
                break;
        }
        
        return std::move(data_recv);
    }
}
