const baseURL = process.env.NODE_ENV === 'production' ? '/ArmoniK.Extensions.Cpp/' : '/'

export default defineNuxtConfig({
  app: {
    baseURL,
    head: {
      link: [
        {
          rel: 'icon',
          type: 'image/ico',
          href: `${baseURL}favicon.ico`
        }
      ]
    }
  },

  extends: '@aneoconsultingfr/armonik-docs-theme',

  runtimeConfig: {
    public: {
      siteName: 'ArmoniK.Extensions.Cpp',
      siteDescription: 'SDK C++ for ArmoniK'
    }
  }
})
