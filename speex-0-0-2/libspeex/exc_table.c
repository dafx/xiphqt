float exc_table[128][8]={{0,0,0,0,0,0,0,0},
{-0.519148,-0.383215,0.492778,1.4744,0.677986,-1.30348,0.654883,0.272205},
{0.235487,1.08995,1.1888,-0.454432,-0.873275,-0.72557,0.127527,1.19132},
{1.31407,-0.12346,-1.6954,1.12853,0.223931,0.316261,0.0103814,0.00687301},
{2.02206,-0.221238,-0.709228,0.133388,-0.0665108,0.644046,-0.811408,0.624665},
{-0.211276,1.64339,-0.628985,-1.14044,1.20664,-0.0699504,-0.0975374,0.408862},
{-0.225241,1.53847,-1.06719,0.295573,-0.476,1.40531,-0.221542,-0.0712951},
{-0.0777728,0.263831,-0.390129,1.28144,-0.732307,-1.00018,1.65573,0.0192973},
{-0.253542,-0.607609,1.56792,-0.386332,0.712034,-1.17144,0.449069,0.902173},
{0.457305,0.159654,0.964737,-1.48001,-0.124851,1.56212,-0.270539,-0.1115},
{-0.535583,1.2753,0.82763,-0.511839,-1.21468,0.387945,1.14347,-0.037292},
{0.658727,0.0951354,-0.91433,1.84221,-1.15651,0.11608,0.457493,-0.0696827},
{-0.171123,-0.640259,1.98106,-0.538926,-0.702949,0.224473,0.146852,0.907132},
{1.23847,-0.735588,-0.458715,1.33074,-1.02485,0.564821,-0.481561,0.815328},
{0.638468,-0.482913,0.360224,1.06189,-1.1122,-0.0272128,1.46374,-0.803822},
{0.142921,0.427225,0.0114741,0.403792,-0.712144,1.23426,1.06859,-1.48582},
{0.206021,-0.025427,0.000886086,-0.300349,1.40432,-1.41072,-0.148592,1.4734},
{0.290391,0.85017,0.246852,0.238577,0.987514,0.876865,-0.322982,-1.54234},
{1.28185,-0.721373,-0.790936,0.761936,0.591477,-0.541731,-0.440675,1.36101},
{-0.44707,1.65442,-0.103376,-0.715018,0.227665,-0.744212,1.38073,-0.0216128},
{1.58882,0.3999,-0.836505,-0.38793,-0.076672,1.06949,0.541261,-0.846127},
{-1.2514,1.17783,0.00721583,0.998346,-0.964169,0.884813,0.160821,0.188163},
{0.032151,-1.41618,0.323532,-0.251319,0.415093,1.07904,0.576218,1.22169},
{-0.14934,1.22495,-0.244964,-0.327759,0.212238,1.15037,-1.46728,0.81144},
{0.0865305,0.447683,-0.253781,-0.475941,1.57808,-1.48456,1.00947,0.0725317},
{1.187,-0.692426,-0.160248,0.402956,0.659311,-1.36626,1.23888,0.053066},
{0.938144,-1.24477,-0.0266074,1.63676,-0.41681,-0.560181,0.552124,0.311536},
{1.19399,0.845415,-0.296021,0.395652,-0.957363,-0.11966,-0.533361,1.44858},
{0.607339,-1.11686,0.895028,0.658016,-0.347789,-0.814287,0.0185571,1.52623},
{1.34549,-0.393902,1.01,-1.15699,-0.63963,0.346319,0.362357,0.948321},
{-0.644387,0.734053,0.336477,-0.812896,1.00561,-0.128355,-0.940326,1.59094},
{-1.49306,0.66011,1.21486,0.115235,0.0322791,-0.66451,0.408269,0.971429},
{0.431509,-0.655904,0.787717,-0.591676,0.704448,-0.350819,-0.882844,1.86331},
{0.274213,1.26762,-1.38245,-0.199402,0.991432,0.00956448,0.865429,-0.678094},
{0.626281,-0.19805,0.260867,-0.0897887,-0.426774,1.47313,-1.6055,1.00902},
{-0.216025,0.0862033,0.445714,-0.0880767,0.301077,-1.3334,1.98526,-0.0555188},
{-0.797843,0.586305,-0.154543,1.70117,1.03544,-0.0184295,-0.334302,-0.381217},
{-1.42492,1.67326,0.405813,-0.506934,0.562486,0.469201,-0.147553,0.0748747},
{0.913965,0.402575,0.682482,0.790534,0.391609,1.16972,0.299276,1.27145},
{-0.640021,0.989951,1.17676,-1.66393,0.355064,0.121597,0.369959,0.403901},
{1.23378,0.641514,-1.22876,0.234667,0.0827593,-0.880098,1.13685,0.527352},
{0.356115,-0.509378,1.05565,-1.04099,0.371755,0.94073,-1.291,1.10064},
{-0.943512,1.31169,-0.390361,-0.103097,1.11716,-1.12928,0.371843,0.818917},
{-0.156317,-0.0418901,0.0723236,0.973233,0.730196,-0.966608,-0.947366,1.5693},
{1.5829,1.33856,0.473229,0.0219714,-0.702468,-0.154917,-0.0369134,-0.470584},
{-0.673854,0.699879,1.12444,-0.662892,-0.71571,1.18947,-0.640153,0.915725},
{0.896011,0.787349,0.200906,-1.46967,0.347642,0.352741,-0.615997,1.25272},
{1.09999,0.792396,-0.884367,-0.727592,0.784642,1.17963,-0.77986,-0.160579},
{-1.05379,-0.0732156,1.59632,-0.132611,-0.169706,1.1097,0.543206,-0.45768},
{0.707575,-0.166165,-0.10517,-0.384722,1.20009,0.203652,-1.55824,1.24602},
{0.426702,-0.750799,-0.785802,0.106489,1.51227,0.957005,0.673555,-0.486873},
{1.28994,-0.960517,0.545345,-0.159149,-0.32678,0.580855,-0.982405,1.49181},
{0.741273,-0.0211484,-1.00192,1.6835,0.0392508,-1.07969,0.231873,0.672362},
{-0.253722,1.1727,1.68946,0.493711,-0.0471442,-0.333803,-0.287764,-0.675685},
{-1.32033,1.11807,0.0833384,0.562357,0.272908,-0.088752,1.31208,-0.778032},
{-0.0803658,0.372868,0.449437,-1.40348,1.39748,0.966697,-0.814686,0.190767},
{0.951774,0.427257,0.15036,-1.60358,1.14627,0.0810234,0.70735,-0.450667},
{1.63808,0.109634,-0.99458,-0.434123,1.35836,-0.260683,-0.111738,0.185745},
{-0.394671,-0.896337,0.646351,1.37455,0.625629,0.804453,0.458131,-0.811909},
{0.0362565,-0.0235451,0.178868,-0.285337,0.0782079,-0.518704,0.189328,2.37162},
{0.492691,1.13895,-0.86741,0.347883,-1.26497,0.380709,1.28738,-0.064377},
{0.930297,1.28741,0.429463,0.245579,0.694034,-1.21605,-0.628591,0.0374087},
{-0.199797,1.13671,-1.09067,1.37141,-0.517385,0.322349,0.764701,-0.852772},
{-0.5007,0.303226,-0.0132675,-0.45596,-0.546562,0.277561,1.5487,1.46029},
{1.65408,-0.842994,0.302966,-0.854034,0.662024,0.910017,-0.687823,0.250232},
{1.12219,-0.5419,-0.336237,-0.655839,-0.373273,0.708425,1.64614,0.265602},
{-0.743476,2.11592,-0.952805,0.264998,0.170145,-0.0560336,0.306247,-0.116348},
{0.403037,-0.48668,0.817853,0.477643,-1.52252,0.964304,-0.546706,1.16473},
{2.11011,-0.537778,-0.497273,0.602666,-0.48884,-0.125965,0.619928,-0.12306},
{0.103031,1.79484,0.0818756,-1.13463,-0.13095,0.917941,0.259381,-0.552635},
{0.428065,-0.557918,1.07281,-0.962703,1.37196,-0.963967,0.901819,-0.278916},
{0.231896,0.041965,0.0632494,1.18255,-1.84059,1.07733,0.269347,-0.00577954},
{0.557442,-1.02095,1.43921,-0.377425,-0.633907,1.32529,-0.709583,0.381231},
{0.518884,1.09369,-1.13942,-0.105153,0.835027,-0.580586,-0.487998,1.39804},
{-1.43902,-0.319956,-0.0826762,0.354559,1.1348,0.807323,0.546867,0.812576},
{1.00645,-0.128206,0.28316,0.334308,-1.33777,-0.533485,1.17966,1.00905},
{0.948813,0.119369,-1.01666,1.23683,0.105635,-0.459294,1.13103,-0.980494},
{1.17283,-1.5799,1.33239,-0.485987,0.211327,0.169073,-0.387067,0.677013},
{-0.247152,-0.0280727,-0.234614,1.39573,-0.963978,-0.0813067,-0.0728599,1.67411},
{-0.449916,-0.274801,1.35576,0.451492,1.43631,0.141543,-0.896003,-0.0876196},
{0.892366,-1.36578,0.444708,0.107565,1.44366,-0.755166,-0.0429619,0.585746},
{0.939511,0.341971,-0.974305,0.928629,-1.00693,1.42408,-0.602308,0.126122},
{-0.5307,-0.23615,1.20135,0.699046,-1.25741,-0.380481,1.25698,0.455196},
{1.04482,1.14309,1.03158,0.939667,0.812349,0.425079,0.423823,0.0813321},
{1.29549,-0.745042,-0.272242,1.15236,-0.838247,1.02629,0.227678,-0.774721},
{0.981011,-1.45622,0.999967,0.725389,-1.11806,0.54881,0.176738,0.191717},
{0.746684,-1.4211,1.37464,0.0883952,0.0254114,-0.573669,1.0495,-0.0720876},
{0.565948,0.269033,-1.10345,1.01211,0.0768689,0.564008,-1.29685,1.19745},
{1.84142,-1.60809,0.161708,0.38437,-0.0217134,0.210367,-0.0739498,0.363587},
{-0.0523482,0.24593,-0.110338,0.493247,0.168289,-1.68577,0.925992,1.40436},
{1.42867,-0.892126,0.262141,0.307225,-0.951306,1.38196,-0.728869,0.319043},
{0.13951,-0.538299,-0.107962,0.370945,0.809012,1.83285,-0.951098,-0.0192296},
{-0.24702,0.992114,-0.865963,0.681391,0.616911,-1.25823,1.37272,-0.405411},
{1.55571,0.0192892,1.45179,0.186482,-0.152518,-0.257762,-0.833094,0.00577311},
{-0.242198,-0.774192,-0.86156,0.991197,0.195455,0.670919,1.44314,0.515926},
{0.405336,-0.228159,1.06259,-0.21443,-1.58614,1.13637,0.811685,-0.236429},
{0.507745,0.074398,0.266039,-0.112387,0.977209,-0.600809,1.43474,-1.4099},
{-0.0917663,0.669177,-0.911825,0.0500723,1.95743,0.0154906,-0.710767,0.243701},
{0.481562,-0.951578,1.72838,-1.24032,0.629132,0.270267,0.185945,-0.0909246},
{1.19697,-0.576168,-0.355474,1.184,1.18151,0.00954522,-0.758864,-0.38764},
{1.36069,-0.965855,0.881144,-0.305908,-0.138624,0.95165,0.470982,-0.962228},
{-0.412674,-0.162429,1.24246,0.215614,-0.190758,0.185485,-1.26035,1.59782},
{1.7233,-1.02853,0.708902,-0.705601,0.736584,-0.463615,0.603755,-0.265125},
{-0.171625,0.994245,-1.16615,1.47341,-0.906766,0.725075,-0.443446,0.507531},
{-0.877189,1.47809,-0.597217,0.392568,-0.194374,0.270966,-0.589283,1.39253},
{0.0658357,0.498163,-0.196594,0.147868,-0.348259,-0.196625,2.11986,-1.03849},
{0.261188,0.170408,0.389773,0.034553,-0.891044,2.07769,-0.549077,-0.455646},
{0.590399,-0.0467183,-1.27306,-0.617851,0.467039,0.87919,0.606851,1.28071},
{-0.897853,0.713074,0.696729,-0.812095,1.4373,-0.592191,0.922986,-0.414167},
{0.54709,0.646744,0.630487,1.50951,-0.345142,0.350881,-1.17514,-0.402886},
{-0.0816486,0.289308,-0.293964,0.86241,-1.05741,1.40111,-1.11131,1.05698},
{0.186736,0.0771509,1.41461,-1.00273,-0.153773,-0.114169,1.4834,-0.634813},
{0.363376,0.423276,-1.35003,0.73783,1.35552,-1.15193,0.292795,0.411996},
{0.731525,-1.33912,0.457722,1.0034,0.0192787,0.532992,-1.18752,0.913092},
{-0.31047,1.2268,-1.553,1.20347,0.0226895,-0.455502,0.438372,0.527228},
{-0.239933,1.39976,0.334577,1.10709,-0.871845,-1.04237,0.37649,0.427001},
{0.971516,1.38103,-1.62783,0.60115,0.073291,0.180237,-0.252818,0.0398674},
{1.10027,1.2861,-0.359542,-1.1075,-0.42857,-0.0905618,0.877396,0.683471},
{-0.625662,-0.595133,1.40743,1.49617,-0.587654,-0.0180775,-0.155266,0.446349},
{0.948704,0.636859,-0.179848,-0.685071,-1.20254,1.30743,0.10664,0.850157},
{-0.097477,0.257188,0.40842,-1.28178,1.83085,-0.690013,-0.17072,0.73336},
{1.88226,-0.462958,0.0975533,-0.398942,0.315353,-0.689253,0.0387385,1.21094},
{0.139807,0.465299,0.548072,0.709881,1.07842,0.885513,1.30961,0.886652},
{-0.294122,0.381301,-0.219781,-0.964707,0.404105,1.58573,1.13793,-0.482169},
{0.377838,-0.0050249,0.535722,-1.15191,0.658768,-1.01305,1.19409,1.1331},
{0.175,-0.0275022,-0.0336047,0.0388552,-0.180907,0.565689,-1.23001,2.13179},
{0.931122,-0.787445,1.06286,-1.30927,1.16088,-0.357942,-0.230584,0.75641},
{-0.451216,-0.159499,1.55692,-1.38334,0.764803,0.101408,-0.382677,0.980935}};
