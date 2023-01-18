package com.hilgenberg.jigsaw
import android.util.Log
import com.android.billingclient.api.*

@Suppress("OverridingDeprecatedMember", "DEPRECATION")
class License(val activity: MainActivity)
	: PurchasesUpdatedListener
	, BillingClientStateListener
	, SkuDetailsResponseListener
	, PurchasesResponseListener
{
	fun buy()
	{
		if (skud == null)
		{
			pendingBuy = true
			Log.e("JIGSAW", "Trying to buy but have no details yet!")
			return
		}
		pendingBuy = false
		billing.launchBillingFlow(activity, BillingFlowParams.newBuilder().setSkuDetails(skud!!).build())
			.takeIf { it.responseCode != BillingClient.BillingResponseCode.OK }
			?.let { Log.e("BillingClient", "Failed to launch billing flow $it") }
	}
	fun check() : Boolean?
	{
		return lic
	}

	private var billing: BillingClient
	private var skud: SkuDetails? = null
	private var lic: Boolean? = null
	private var pendingBuy: Boolean = false

	init
	{
		billing = BillingClient.newBuilder(activity).setListener(this).enablePendingPurchases().build()
		billing.startConnection(this)
	}

	override fun onPurchasesUpdated(r: BillingResult, P: MutableList<Purchase>?)
	{
		when (r.responseCode)
		{
			BillingClient.BillingResponseCode.OK -> P?.forEach {
				if (it.purchaseState != Purchase.PurchaseState.PURCHASED) return@forEach
				billing.acknowledgePurchase(AcknowledgePurchaseParams.newBuilder().setPurchaseToken(it.purchaseToken).build()) { r2 ->
					if (r2.responseCode == BillingClient.BillingResponseCode.OK)
					{
						lic = true
						Log.d("JIGSAW", "Purchase acknowledged.")
					} else {
						Log.e("JIGSAW", "Failed to acknowledge purchase $r2")
					}
				}
			}
			BillingClient.BillingResponseCode.ITEM_ALREADY_OWNED -> queryPurchases()
			BillingClient.BillingResponseCode.SERVICE_DISCONNECTED -> onBillingServiceDisconnected()
			else -> Log.e("JIGSAW", "Failed with code ${r.responseCode}")
		}
	}
	override fun onBillingServiceDisconnected()
	{
		Log.d("JIGSAW", "Play Store disconnected - reconnecting")
		if (!billing.isReady) billing.startConnection(this)
	}
	override fun onBillingSetupFinished(r: BillingResult)
	{
		if (r.responseCode != BillingClient.BillingResponseCode.OK) {
			Log.e("JIGSAW", "Play Store failed with code: $r")
			return
		}
		Log.d("JIGSAW", "Play Store connected")
		billing.querySkuDetailsAsync(SkuDetailsParams.newBuilder()
			.setType(BillingClient.SkuType.INAPP)
			.setSkusList(List(1){"full_version"})
			.build(), this) // calls onSkuDetailsResponse
		queryPurchases()
	}

	override fun onSkuDetailsResponse(r: BillingResult, D: MutableList<SkuDetails>?)
	{
		Log.d("JIGSAW", "SKU details arrived: $D, code $r")
		if (D != null && D.isNotEmpty()) for (d in D) {
			if (d.sku != "full_version") continue
			skud = d
			Log.d("JIGSAW", "Found $skud")

		}
		if (pendingBuy) buy()
	}

	private fun queryPurchases() {
		billing.queryPurchasesAsync(BillingClient.SkuType.INAPP, this)
	}

	override fun onQueryPurchasesResponse(r: BillingResult, P: List<Purchase?>)
	{
		if (r.responseCode != BillingClient.BillingResponseCode.OK) {
			Log.e("JIGSAW", "Play Store get_purchases failed with code: ${r.responseCode}")
			return
		}

		Log.d("JIGSAW", "Play Store sent purchases: $P")
		lic = false
		for (p in P)
		{
			if (p == null || p.purchaseState != Purchase.PurchaseState.PURCHASED) continue
			p.products.forEach { if (it == "full_version") lic = true }
		}
	}
}