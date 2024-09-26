const WID_SERVER_URL = 'https://api.wid-timer.org/signin';

export default class Service {

  async post<Type>(endpoint:string, data: Type) {
    try {
      const response = await fetch(`${WID_SERVER_URL}${endpoint}`, {
        method: "POST",
        mode: "cors",
        credentials: "include",
        headers: {
          Accept: "application/json",
          "Content-Type": "application/json",
        },
        body: JSON.stringify(data),
      });

      let result:string = '';
      if (response.status === 401) {
        // Notify not authenticated
        return Promise.reject({ status: response.status });
      } else if (response.status !== 204) {
        result = await response.json();
      } else {
        result = '';
      }

      if (response.ok) {
        return Promise.resolve(result);
      } else {
        throw response.status;
      }
    } catch (e) {
      console.error(e);
      return Promise.reject({exception: e});
    }
  }

}
