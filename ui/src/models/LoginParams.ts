import { reactive, Reactive } from 'vue';

export default class LoginParams {
  email: string;
  password: string;

  constructor(email: string, password: string) {
    this.email = email;
    this.password = password;
  }

  static newReactive(email: string, password: string): Reactive<LoginParams> {
    return reactive<LoginParams>(new LoginParams(email, password));
  }

  isValid(): boolean {
    return this.email?.length > 0 && this.password?.length > 0;
  }
}
