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
	private var billing:    BillingClient
	private var product:    SkuDetails? = null
	private var licensed:   Boolean?    = null
	private var pendingBuy: Boolean     = false

	init
	{
		billing = BillingClient.newBuilder(activity).setListener(this).enablePendingPurchases().build()
		billing.startConnection(this)
	}

	fun check() : Boolean?
	{
		return licensed
	}

	fun buy()
	{
		if (product == null)
		{
			pendingBuy = true
			Log.e("JIGSAW", "Trying to buy but have no details yet!")
			return
		}
		pendingBuy = false
		billing.launchBillingFlow(activity, BillingFlowParams.newBuilder().setSkuDetails(product!!).build())
			.takeIf { it.responseCode != BillingClient.BillingResponseCode.OK }
			?.let { Log.e("BillingClient", "Failed to launch billing flow $it") }
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
						val wasLicensed = (licensed==true)
						licensed = true
						Log.d("JIGSAW", "Purchase acknowledged.")
						if (!wasLicensed) activity.licenseChanged()
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
			product = d
			Log.d("JIGSAW", "Found $product")

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

		val wasLicensed = (licensed==true)
		Log.d("JIGSAW", "Play Store sent purchases: $P")
		licensed = false
		for (p in P)
		{
			if (p == null || p.purchaseState != Purchase.PurchaseState.PURCHASED) continue
			p.products.forEach { if (it == "full_version") {
				licensed = true
				if (!wasLicensed) activity.licenseChanged()
			} }
		}
	}
}