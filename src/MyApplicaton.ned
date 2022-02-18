
package veins_myproject.src;


import org.car2x.veins.modules.application.ieee80211p.BaseWaveApplLayer;

simple MyApplicaton extends BaseWaveApplLayer
{
    //declarations go here
    @class(MyApplicaton);
    @display("i=block/segm");
    @signal[delay](type="float");
    @statistic[delay](title="delay"; source="delay"; record=mean, sum, stats, vector);
}
