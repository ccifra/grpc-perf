
//---------------------------------------------------------------------
//---------------------------------------------------------------------
class NIPerfTestClient;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformMessagePerformanceTest(NIPerfTestClient& client);
void PerformLatencyStreamTest(NIPerfTestClient& client, std::string fileName);
void PerformMonikerLatencyReadWriteTest(NIMonikerClient& client, int numItems, bool useAnyValue, std::string fileName);
void PerformLatencyPayloadWriteTest(NIPerfTestClient& client, int numSamples, std::string fileName);
void PerformLatencyPayloadWriteStreamTest(NIPerfTestClient& client, int numSamples, std::string fileName);
void PerformLatencyStreamTest2(NIPerfTestClient& client, NIPerfTestClient& client2, int streamCount, std::string fileName);
void PerformMessageLatencyTest(NIPerfTestClient& client, std::string fileName);
void PerformReadTest(NIPerfTestClient& client, int numSamples);
void PerformWriteTest(NIPerfTestClient& client, int numSamples);
void PerformStreamingTest(NIPerfTestClient& client, int numSamples);
void PerformTwoStreamTest(NIPerfTestClient& client, NIPerfTestClient& client2, int numSamples);
void PerformFourStreamTest(NIPerfTestClient& client, NIPerfTestClient& client2, NIPerfTestClient& client3, NIPerfTestClient& client4, int numSamples);
void PerformNStreamTest(std::vector<NIPerfTestClient*>& clients, int numSamples);
